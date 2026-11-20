/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

// libsource.so (MediaTek video-call camera source, used by vtservice) was built against
// Android 15 libgui. Two things it depends on changed shape in Android 16:
//
// 1. android::BufferItem grew from 0x118 to 0x128 bytes: std::optional<PictureProfileHandle>
//    mPictureProfileHandle was inserted at 0x98. VTCameraSource embeds a BufferItem with only
//    0x118 bytes reserved, and passes BufferItem* to IGraphicBufferConsumer::acquireBuffer.
//
// 2. IGraphicBufferConsumer is no longer a binder IInterface (public virtual RefBase) but a
//    plain RefBase, and its vtable gained getReleasedBuffersExtended/allowUnlimitedSlots.
//    libsource's sp<IGraphicBufferConsumer> code looks the RefBase up through the vbase
//    offset in the vtable, which the A16 class doesn't have, so the decStrong in
//    ~VTCameraSource read a pointer from the preceding vtable as the offset and crashed
//    vtservice at the end of every call. Its virtual calls hit the wrong slots too.
//
// extract-files.py renames libsource's imports (same length, "android" -> "md07pvt") of
// BufferItem::BufferItem(), ~BufferItem() and BufferQueue::createBufferQueue() to the
// functions below. BufferItems are converted between the two layouts, and libsource gets an
// adapter with the A15 IGraphicBufferConsumer shape that forwards to the real A16 consumer.

#include <gui/BufferItem.h>
#include <gui/BufferQueue.h>
#include <gui/IConsumerListener.h>
#include <gui/IGraphicBufferConsumer.h>
#include <gui/IGraphicBufferProducer.h>
#include <log/log.h>

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <new>

using android::BufferItem;
using android::IGraphicBufferConsumer;
using android::IGraphicBufferProducer;
using android::RefBase;
using android::sp;
using android::status_t;

namespace {

constexpr size_t kA15Size = 0x118;
constexpr size_t kHandleOffset = 0x98;  // mPictureProfileHandle in A16 (absent in A15)
constexpr size_t kHandleSize = 0x10;
constexpr size_t kTailSize = kA15Size - kHandleOffset;  // mFrameNumber .. mApi

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-offsetof"
static_assert(sizeof(BufferItem) == kA15Size + kHandleSize, "unexpected BufferItem size");
static_assert(offsetof(BufferItem, mPictureProfileHandle) == kHandleOffset,
              "unexpected mPictureProfileHandle offset");
static_assert(offsetof(BufferItem, mFrameNumber) == kHandleOffset + kHandleSize,
              "unexpected mFrameNumber offset");
#pragma clang diagnostic pop

struct A16Storage {
    alignas(BufferItem) unsigned char bytes[sizeof(BufferItem)];
    BufferItem* item() { return reinterpret_cast<BufferItem*>(bytes); }
};

// Moves an A15-layout item into tmp (bit-wise; ownership of its members moves with it).
void unpack(const void* a15, A16Storage* tmp) {
    auto* src = static_cast<const unsigned char*>(a15);
    memcpy(tmp->bytes, src, kHandleOffset);
    new (tmp->bytes + kHandleOffset) std::optional<android::PictureProfileHandle>();
    memcpy(tmp->bytes + kHandleOffset + kHandleSize, src + kHandleOffset, kTailSize);
}

// Moves tmp back into A15 layout. tmp must not be destroyed afterwards.
void pack(A16Storage* tmp, void* a15) {
    tmp->item()->mPictureProfileHandle.reset();
    auto* dst = static_cast<unsigned char*>(a15);
    memcpy(dst, tmp->bytes, kHandleOffset);
    memcpy(dst + kHandleOffset, tmp->bytes + kHandleOffset + kHandleSize, kTailSize);
}

// Has the vtable shape of the Android 15 IGraphicBufferConsumer, so the virtual calls and
// sp<> refcounting compiled into libsource land where it expects. Do not reorder.
class A15GraphicBufferConsumer : public virtual RefBase {
  public:
    explicit A15GraphicBufferConsumer(const sp<IGraphicBufferConsumer>& consumer)
        : mConsumer(consumer) {}

    // IInterface
    ~A15GraphicBufferConsumer() override = default;                // 0x00, 0x08
    virtual void* onAsBinder() { unsupported("onAsBinder"); }      // 0x10
    virtual const void* getInterfaceDescriptor() const {           // 0x18
        unsupported("getInterfaceDescriptor");
    }

    // IGraphicBufferConsumer (A15 order)
    virtual status_t acquireBuffer(void* a15Item, nsecs_t presentWhen,  // 0x20
                                   uint64_t maxFrameNumber) {
        A16Storage tmp;
        unpack(a15Item, &tmp);
        status_t ret = mConsumer->acquireBuffer(tmp.item(), presentWhen, maxFrameNumber);
        pack(&tmp, a15Item);
        return ret;
    }
    virtual status_t detachBuffer(int slot) { return mConsumer->detachBuffer(slot); }
    virtual status_t attachBuffer(int* outSlot, const sp<android::GraphicBuffer>& buffer) {
        return mConsumer->attachBuffer(outSlot, buffer);
    }
    virtual status_t releaseBuffer(int buf, uint64_t frameNumber, EGLDisplay display,  // 0x38
                                   EGLSyncKHR fence, const sp<android::Fence>& releaseFence) {
        return mConsumer->releaseBuffer(buf, frameNumber, display, fence, releaseFence);
    }
    virtual status_t consumerConnect(const sp<android::IConsumerListener>& consumer,  // 0x40
                                     bool controlledByApp) {
        return mConsumer->consumerConnect(consumer, controlledByApp);
    }
    virtual status_t consumerDisconnect() { return mConsumer->consumerDisconnect(); }
    virtual status_t getReleasedBuffers(uint64_t* slotMask) {
        return mConsumer->getReleasedBuffers(slotMask);
    }
    virtual status_t setDefaultBufferSize(uint32_t w, uint32_t h) {  // 0x58
        return mConsumer->setDefaultBufferSize(w, h);
    }
    virtual status_t setMaxBufferCount(int bufferCount) {
        return mConsumer->setMaxBufferCount(bufferCount);
    }
    virtual status_t setMaxAcquiredBufferCount(int maxAcquiredBuffers) {  // 0x68
        return mConsumer->setMaxAcquiredBufferCount(maxAcquiredBuffers);
    }
    virtual status_t setConsumerName(const android::String8& name) {  // 0x70
        return mConsumer->setConsumerName(name);
    }
    virtual status_t setDefaultBufferFormat(android::PixelFormat format) {
        return mConsumer->setDefaultBufferFormat(format);
    }
    virtual status_t setDefaultBufferDataSpace(android_dataspace dataSpace) {
        return mConsumer->setDefaultBufferDataSpace(dataSpace);
    }
    virtual status_t setConsumerUsageBits(uint64_t usage) {  // 0x88
        return mConsumer->setConsumerUsageBits(usage);
    }
    virtual status_t setConsumerIsProtected(bool isProtected) {
        return mConsumer->setConsumerIsProtected(isProtected);
    }
    virtual status_t setTransformHint(uint32_t hint) { return mConsumer->setTransformHint(hint); }
    virtual status_t getSidebandStream(sp<android::NativeHandle>* outStream) const {
        return mConsumer->getSidebandStream(outStream);
    }
    virtual status_t getOccupancyHistory(bool, void*) { unsupported("getOccupancyHistory"); }
    virtual status_t discardFreeBuffers() { return mConsumer->discardFreeBuffers(); }
    virtual status_t dumpState(const android::String8& prefix, android::String8* outResult) const {
        return mConsumer->dumpState(prefix, outResult);
    }

  private:
    [[noreturn]] static void unsupported(const char* what) {
        LOG_ALWAYS_FATAL("libshim_vtsource: A15 IGraphicBufferConsumer::%s not supported", what);
    }

    const sp<IGraphicBufferConsumer> mConsumer;
};

}  // namespace

extern "C" void md07p_vt_BufferItem_ctor(void* a15) __asm__("_ZN7md07pvt10BufferItemC1Ev");
extern "C" void md07p_vt_BufferItem_dtor(void* a15) __asm__("_ZN7md07pvt10BufferItemD1Ev");
extern "C" void md07p_vt_createBufferQueue(sp<IGraphicBufferProducer>* outProducer,
                                           void* outConsumer, bool consumerIsSurfaceFlinger)
        __asm__("_ZN7md07pvt11BufferQueue17createBufferQueueEPNS_2spINS_22IGraphicBufferProducerEEEPNS1_INS_22IGraphicBufferConsumerEEEb");

extern "C" void md07p_vt_BufferItem_ctor(void* a15) {
    A16Storage tmp;
    new (tmp.bytes) BufferItem();
    pack(&tmp, a15);
}

extern "C" void md07p_vt_BufferItem_dtor(void* a15) {
    A16Storage tmp;
    unpack(a15, &tmp);
    tmp.item()->~BufferItem();
}

extern "C" void md07p_vt_createBufferQueue(sp<IGraphicBufferProducer>* outProducer,
                                           void* outConsumer, bool consumerIsSurfaceFlinger) {
    sp<IGraphicBufferConsumer> consumer;
    android::BufferQueue::createBufferQueue(outProducer, &consumer, consumerIsSurfaceFlinger);

    // outConsumer is libsource's sp<A15 IGraphicBufferConsumer>: a single pointer.
    auto** out = static_cast<A15GraphicBufferConsumer**>(outConsumer);
    A15GraphicBufferConsumer* old = *out;
    A15GraphicBufferConsumer* adapter = nullptr;
    if (consumer != nullptr) {
        adapter = new A15GraphicBufferConsumer(consumer);
        adapter->incStrong(out);
    }
    *out = adapter;
    if (old != nullptr) old->decStrong(out);
}
