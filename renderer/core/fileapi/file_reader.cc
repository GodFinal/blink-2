/*
 * Copyright (C) 2010 Google Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/fileapi/file_reader.h"

#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/bindings/core/v8/string_or_array_buffer.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/core/events/progress_event.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/fileapi/file.h"
#include "third_party/blink/renderer/core/frame/use_counter.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/core/typed_arrays/dom_array_buffer.h"
#include "third_party/blink/renderer/platform/supplementable.h"
#include "third_party/blink/renderer/platform/wtf/auto_reset.h"
#include "third_party/blink/renderer/platform/wtf/deque.h"
#include "third_party/blink/renderer/platform/wtf/hash_set.h"
#include "third_party/blink/renderer/platform/wtf/text/cstring.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

namespace {

const CString Utf8BlobUUID(Blob* blob) {
  return blob->Uuid().Utf8();
}

const CString Utf8FilePath(Blob* blob) {
  return blob->HasBackingFile() ? ToFile(blob)->GetPath().Utf8() : "";
}

}  // namespace

// Embedders like chromium limit the number of simultaneous requests to avoid
// excessive IPC congestion. We limit this to 100 per thread to throttle the
// requests (the value is arbitrarily chosen).
static const size_t kMaxOutstandingRequestsPerThread = 100;
static const double kProgressNotificationIntervalMS = 50;

class FileReader::ThrottlingController final
    : public GarbageCollected<FileReader::ThrottlingController>,
      public Supplement<ExecutionContext> {
  USING_GARBAGE_COLLECTED_MIXIN(FileReader::ThrottlingController);

 public:
  static const char kSupplementName[];

  static ThrottlingController* From(ExecutionContext* context) {
    if (!context)
      return nullptr;

    ThrottlingController* controller =
        Supplement<ExecutionContext>::From<ThrottlingController>(*context);
    if (!controller) {
      controller = new ThrottlingController(*context);
      ProvideTo(*context, controller);
    }
    return controller;
  }

  enum FinishReaderType { kDoNotRunPendingReaders, kRunPendingReaders };

  static void PushReader(ExecutionContext* context, FileReader* reader) {
    ThrottlingController* controller = From(context);
    if (!controller)
      return;

    probe::AsyncTaskScheduled(context, "FileReader", reader);
    controller->PushReader(reader);
  }

  static FinishReaderType RemoveReader(ExecutionContext* context,
                                       FileReader* reader) {
    ThrottlingController* controller = From(context);
    if (!controller)
      return kDoNotRunPendingReaders;

    return controller->RemoveReader(reader);
  }

  static void FinishReader(ExecutionContext* context,
                           FileReader* reader,
                           FinishReaderType next_step) {
    ThrottlingController* controller = From(context);
    if (!controller)
      return;

    controller->FinishReader(reader, next_step);
    probe::AsyncTaskCanceled(context, reader);
  }

  void Trace(blink::Visitor* visitor) {
    visitor->Trace(pending_readers_);
    visitor->Trace(running_readers_);
    Supplement<ExecutionContext>::Trace(visitor);
  }

 private:
  explicit ThrottlingController(ExecutionContext& context)
      : Supplement<ExecutionContext>(context),
        max_running_readers_(kMaxOutstandingRequestsPerThread) {}

  void PushReader(FileReader* reader) {
    if (pending_readers_.IsEmpty() &&
        running_readers_.size() < max_running_readers_) {
      reader->ExecutePendingRead();
      DCHECK(!running_readers_.Contains(reader));
      running_readers_.insert(reader);
      return;
    }
    pending_readers_.push_back(reader);
    ExecuteReaders();
  }

  FinishReaderType RemoveReader(FileReader* reader) {
    FileReaderHashSet::const_iterator hash_iter = running_readers_.find(reader);
    if (hash_iter != running_readers_.end()) {
      running_readers_.erase(hash_iter);
      return kRunPendingReaders;
    }
    FileReaderDeque::const_iterator deque_end = pending_readers_.end();
    for (FileReaderDeque::const_iterator it = pending_readers_.begin();
         it != deque_end; ++it) {
      if (*it == reader) {
        pending_readers_.erase(it);
        break;
      }
    }
    return kDoNotRunPendingReaders;
  }

  void FinishReader(FileReader* reader, FinishReaderType next_step) {
    if (next_step == kRunPendingReaders)
      ExecuteReaders();
  }

  void ExecuteReaders() {
    while (running_readers_.size() < max_running_readers_) {
      if (pending_readers_.IsEmpty())
        return;
      FileReader* reader = pending_readers_.TakeFirst();
      reader->ExecutePendingRead();
      running_readers_.insert(reader);
    }
  }

  const size_t max_running_readers_;

  using FileReaderDeque = HeapDeque<Member<FileReader>>;
  using FileReaderHashSet = HeapHashSet<Member<FileReader>>;

  FileReaderDeque pending_readers_;
  FileReaderHashSet running_readers_;
};

// static
const char FileReader::ThrottlingController::kSupplementName[] =
    "FileReaderThrottlingController";

FileReader* FileReader::Create(ExecutionContext* context) {
  return new FileReader(context);
}

FileReader::FileReader(ExecutionContext* context)
    : ContextLifecycleObserver(context),
      state_(kEmpty),
      loading_state_(kLoadingStateNone),
      still_firing_events_(false),
      read_type_(FileReaderLoader::kReadAsBinaryString),
      last_progress_notification_time_ms_(0) {}

FileReader::~FileReader() {
  Terminate();
}

const AtomicString& FileReader::InterfaceName() const {
  return EventTargetNames::FileReader;
}

void FileReader::ContextDestroyed(ExecutionContext* destroyed_context) {
  // The delayed abort task tidies up and advances to the DONE state.
  if (loading_state_ == kLoadingStateAborted)
    return;

  if (HasPendingActivity()) {
    ThrottlingController::FinishReader(
        destroyed_context, this,
        ThrottlingController::RemoveReader(destroyed_context, this));
  }
  Terminate();
}

bool FileReader::HasPendingActivity() const {
  return state_ == kLoading || still_firing_events_;
}

void FileReader::readAsArrayBuffer(Blob* blob,
                                   ExceptionState& exception_state) {
  DCHECK(blob);
  DVLOG(1) << "reading as array buffer: " << Utf8BlobUUID(blob).data() << " "
           << Utf8FilePath(blob).data();

  ReadInternal(blob, FileReaderLoader::kReadAsArrayBuffer, exception_state);
}

void FileReader::readAsBinaryString(Blob* blob,
                                    ExceptionState& exception_state) {
  DCHECK(blob);
  DVLOG(1) << "reading as binary: " << Utf8BlobUUID(blob).data() << " "
           << Utf8FilePath(blob).data();

  ReadInternal(blob, FileReaderLoader::kReadAsBinaryString, exception_state);
}

void FileReader::readAsText(Blob* blob,
                            const String& encoding,
                            ExceptionState& exception_state) {
  DCHECK(blob);
  DVLOG(1) << "reading as text: " << Utf8BlobUUID(blob).data() << " "
           << Utf8FilePath(blob).data();

  encoding_ = encoding;
  ReadInternal(blob, FileReaderLoader::kReadAsText, exception_state);
}

void FileReader::readAsText(Blob* blob, ExceptionState& exception_state) {
  readAsText(blob, String(), exception_state);
}

void FileReader::readAsDataURL(Blob* blob, ExceptionState& exception_state) {
  DCHECK(blob);
  DVLOG(1) << "reading as data URL: " << Utf8BlobUUID(blob).data() << " "
           << Utf8FilePath(blob).data();

  ReadInternal(blob, FileReaderLoader::kReadAsDataURL, exception_state);
}

void FileReader::ReadInternal(Blob* blob,
                              FileReaderLoader::ReadType type,
                              ExceptionState& exception_state) {
  // If multiple concurrent read methods are called on the same FileReader,
  // InvalidStateError should be thrown when the state is kLoading.
  if (state_ == kLoading) {
    exception_state.ThrowDOMException(
        kInvalidStateError, "The object is already busy reading Blobs.");
    return;
  }

  ExecutionContext* context = GetExecutionContext();
  if (!context) {
    exception_state.ThrowDOMException(
        kAbortError, "Reading from a detached FileReader is not supported.");
    return;
  }

  // A document loader will not load new resources once the Document has
  // detached from its frame.
  if (context->IsDocument() && !ToDocument(context)->GetFrame()) {
    exception_state.ThrowDOMException(
        kAbortError,
        "Reading from a Document-detached FileReader is not supported.");
    return;
  }

  // "Snapshot" the Blob data rather than the Blob itself as ongoing
  // read operations should not be affected if close() is called on
  // the Blob being read.
  blob_data_handle_ = blob->GetBlobDataHandle();
  blob_type_ = blob->type();
  read_type_ = type;
  state_ = kLoading;
  loading_state_ = kLoadingStatePending;
  error_ = nullptr;
  DCHECK(ThrottlingController::From(context));
  ThrottlingController::PushReader(context, this);
}

void FileReader::ExecutePendingRead() {
  DCHECK_EQ(loading_state_, kLoadingStatePending);
  loading_state_ = kLoadingStateLoading;

  loader_ = FileReaderLoader::Create(read_type_, this);
  loader_->SetEncoding(encoding_);
  loader_->SetDataType(blob_type_);
  loader_->Start(blob_data_handle_);
  blob_data_handle_ = nullptr;
}

void FileReader::abort() {
  DVLOG(1) << "aborting";

  if (loading_state_ != kLoadingStateLoading &&
      loading_state_ != kLoadingStatePending) {
    return;
  }
  loading_state_ = kLoadingStateAborted;

  DCHECK_NE(kDone, state_);
  state_ = kDone;

  AutoReset<bool> firing_events(&still_firing_events_, true);

  // Setting error implicitly makes |result| return null.
  error_ = FileError::CreateDOMException(FileError::kAbortErr);

  // Unregister the reader.
  ThrottlingController::FinishReaderType final_step =
      ThrottlingController::RemoveReader(GetExecutionContext(), this);

  FireEvent(EventTypeNames::abort);
  FireEvent(EventTypeNames::loadend);

  // All possible events have fired and we're done, no more pending activity.
  ThrottlingController::FinishReader(GetExecutionContext(), this, final_step);

  // ..but perform the loader cancellation asynchronously as abort() could be
  // called from the event handler and we do not want the resource loading code
  // to be on the stack when doing so. The persistent reference keeps the
  // reader alive until the task has completed.
  GetExecutionContext()
      ->GetTaskRunner(TaskType::kFileReading)
      ->PostTask(FROM_HERE,
                 WTF::Bind(&FileReader::Terminate, WrapPersistent(this)));
}

void FileReader::result(ScriptState* state,
                        StringOrArrayBuffer& result_attribute) const {
  if (error_ || !loader_)
    return;

  if (!loader_->HasFinishedLoading()) {
    UseCounter::Count(ExecutionContext::From(state),
                      WebFeature::kFileReaderResultBeforeCompletion);
  }

  if (read_type_ == FileReaderLoader::kReadAsArrayBuffer)
    result_attribute.SetArrayBuffer(loader_->ArrayBufferResult());
  else
    result_attribute.SetString(loader_->StringResult());
}

void FileReader::Terminate() {
  if (loader_) {
    loader_->Cancel();
    loader_ = nullptr;
  }
  state_ = kDone;
  loading_state_ = kLoadingStateNone;
}

void FileReader::DidStartLoading() {
  AutoReset<bool> firing_events(&still_firing_events_, true);
  FireEvent(EventTypeNames::loadstart);
}

void FileReader::DidReceiveData() {
  // Fire the progress event at least every 50ms.
  double now = CurrentTimeMS();
  if (!last_progress_notification_time_ms_) {
    last_progress_notification_time_ms_ = now;
  } else if (now - last_progress_notification_time_ms_ >
             kProgressNotificationIntervalMS) {
    AutoReset<bool> firing_events(&still_firing_events_, true);
    FireEvent(EventTypeNames::progress);
    last_progress_notification_time_ms_ = now;
  }
}

void FileReader::DidFinishLoading() {
  if (loading_state_ == kLoadingStateAborted)
    return;
  DCHECK_EQ(loading_state_, kLoadingStateLoading);

  // TODO(jochen): When we set m_state to DONE below, we still need to fire
  // the load and loadend events. To avoid GC to collect this FileReader, we
  // use this separate variable to keep the wrapper of this FileReader alive.
  // An alternative would be to keep any ActiveScriptWrappables alive that is on
  // the stack.
  AutoReset<bool> firing_events(&still_firing_events_, true);

  // It's important that we change m_loadingState before firing any events
  // since any of the events could call abort(), which internally checks
  // if we're still loading (therefore we need abort process) or not.
  loading_state_ = kLoadingStateNone;

  FireEvent(EventTypeNames::progress);

  DCHECK_NE(kDone, state_);
  state_ = kDone;

  // Unregister the reader.
  ThrottlingController::FinishReaderType final_step =
      ThrottlingController::RemoveReader(GetExecutionContext(), this);

  FireEvent(EventTypeNames::load);
  FireEvent(EventTypeNames::loadend);

  // All possible events have fired and we're done, no more pending activity.
  ThrottlingController::FinishReader(GetExecutionContext(), this, final_step);
}

void FileReader::DidFail(FileError::ErrorCode error_code) {
  if (loading_state_ == kLoadingStateAborted)
    return;

  AutoReset<bool> firing_events(&still_firing_events_, true);

  DCHECK_EQ(kLoadingStateLoading, loading_state_);
  loading_state_ = kLoadingStateNone;

  DCHECK_NE(kDone, state_);
  state_ = kDone;

  error_ = FileError::CreateDOMException(error_code);

  // Unregister the reader.
  ThrottlingController::FinishReaderType final_step =
      ThrottlingController::RemoveReader(GetExecutionContext(), this);

  FireEvent(EventTypeNames::error);
  FireEvent(EventTypeNames::loadend);

  // All possible events have fired and we're done, no more pending activity.
  ThrottlingController::FinishReader(GetExecutionContext(), this, final_step);
}

void FileReader::FireEvent(const AtomicString& type) {
  probe::AsyncTask async_task(GetExecutionContext(), this, "event");
  if (!loader_) {
    DispatchEvent(ProgressEvent::Create(type, false, 0, 0));
    return;
  }

  if (loader_->TotalBytes()) {
    DispatchEvent(ProgressEvent::Create(type, true, loader_->BytesLoaded(),
                                        *loader_->TotalBytes()));
  } else {
    DispatchEvent(
        ProgressEvent::Create(type, false, loader_->BytesLoaded(), 0));
  }
}

void FileReader::Trace(blink::Visitor* visitor) {
  visitor->Trace(error_);
  EventTargetWithInlineData::Trace(visitor);
  ContextLifecycleObserver::Trace(visitor);
}

}  // namespace blink
