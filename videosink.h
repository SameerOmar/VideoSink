//-----------------------------------------------------------------------------
// File: VideoeSink.h
// Description: Archive sink for creating .Video files.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
//  Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

// utility functions

#define USE_LOGGING

#include "common\CritSec.h"
#include "common\AsyncCB.h"
#include "common\LinkList.h"
#include "VideoRecorder_h.h"





// Forward declares
//class CVideoStream;
//class CMarker;

enum FlushState
{
    DropSamples = 0,
    WriteSamples 
};


// IMarker:
// Custom interface for handling IMFStreamSink::PlaceMarker calls asynchronously.

// A marker consists of a marker type, marker data, and context data. 
// By defining this interface, we can store the marker data inside an IUnknown object
// and keep that object on the same queue that holds the media samples. This is 
// useful because samples and markers must be serialized. That is, we cannot respond
// to a marker until we have processed all of the samples that came before it.

// Note that IMarker is not a standard Media Foundation interface.
MIDL_INTERFACE("3AC82233-933C-43a9-AF3D-ADC94EABF406")
IMarker : public IUnknown
{
    virtual STDMETHODIMP GetMarkerType(MFSTREAMSINK_MARKER_TYPE *pType) = 0;
    virtual STDMETHODIMP GetMarkerValue(PROPVARIANT *pvar) = 0;
    virtual STDMETHODIMP GetContext(PROPVARIANT *pvar) = 0;
};



// {BCA19BBF-B335-4406-9357-F138D5B98CF3}
DEFINE_GUID(CLSID_CVideoSink, 
0xbca19bbf, 0xb335, 0x4406, 0x93, 0x57, 0xf1, 0x38, 0xd5, 0xb9, 0x8c, 0xf3);

namespace VideoRecorder { 
class DECLSPEC_UUID("BCA19BBF-B335-4406-9357-F138D5B98CF3") CVideoSink
    : public Microsoft::WRL::RuntimeClass<
           Microsoft::WRL::RuntimeClassFlags< Microsoft::WRL::RuntimeClassType::WinRtClassicComMix >, 
           ABI::VideoRecorder::IVideoSink,
           ABI::Windows::Media::IMediaExtension,
           FtmBase,
           IMFMediaSink,
           IMFClockStateSink >
{
    InspectableClass(RuntimeClass_VideoRecorder_VideoSink,BaseTrust)

    friend class CVideoStream;

public:
    // Static method to create the object.
    //static HRESULT CreateInstance(IMFByteStream *pStream, REFIID iid, void **ppSink);
	IFACEMETHOD (Initialize)();

    //// IUnknown
    //STDMETHODIMP QueryInterface(REFIID iid, void** ppv);
    //STDMETHODIMP_(ULONG) AddRef();
    //STDMETHODIMP_(ULONG) Release();

	// IMediaExtension
    IFACEMETHOD (SetProperties) (ABI::Windows::Foundation::Collections::IPropertySet *pConfiguration) {return S_OK;}

    // IMFMediaSink methods
    IFACEMETHOD (GetCharacteristics)(DWORD *pdwCharacteristics);

    IFACEMETHOD (AddStreamSink)( 
        /* [in] */ DWORD dwStreamSinkIdentifier,
        /* [in] */ IMFMediaType *pMediaType,
        /* [out] */ IMFStreamSink **ppStreamSink);

    IFACEMETHOD (RemoveStreamSink)(DWORD dwStreamSinkIdentifier);
    IFACEMETHOD (GetStreamSinkCount)(DWORD *pcStreamSinkCount);
    IFACEMETHOD (GetStreamSinkByIndex)(DWORD dwIndex, IMFStreamSink **ppStreamSink);
    IFACEMETHOD (GetStreamSinkById)(DWORD dwIdentifier, IMFStreamSink **ppStreamSink);
    IFACEMETHOD (SetPresentationClock)(IMFPresentationClock *pPresentationClock);
    IFACEMETHOD (GetPresentationClock)(IMFPresentationClock **ppPresentationClock);
    IFACEMETHOD (Shutdown)();

    // IMFFinalizableMediaSink methods
    IFACEMETHOD (BeginFinalize)(IMFAsyncCallback *pCallback, IUnknown *punkState);
    IFACEMETHOD (EndFinalize)(IMFAsyncResult *pResult);

    // IMFClockStateSink methods
    IFACEMETHOD (OnClockStart)(MFTIME hnsSystemTime, LONGLONG llClockStartOffset);
    IFACEMETHOD (OnClockStop)(MFTIME hnsSystemTime);
    IFACEMETHOD (OnClockPause)(MFTIME hnsSystemTime);
    IFACEMETHOD (OnClockRestart)(MFTIME hnsSystemTime);
    IFACEMETHOD (OnClockSetRate)(MFTIME hnsSystemTime, float flRate);
    
	CVideoSink();
    ~CVideoSink();

private:


    HRESULT Initialize(IMFByteStream *pByteStream);

    HRESULT CheckShutdown() const
    {
        if (m_IsShutdown)
        {
            return MF_E_SHUTDOWN;
        }
        else
        {
            return S_OK;
        }
    }


    long                        m_nRefCount;                // reference count
    CritSec                     m_critSec;                  // critical section for thread safety

    BOOL                        m_IsShutdown;               // Flag to indicate if Shutdown() method was called.

    CVideoStream                  *m_pStream;                 // Byte stream
    IMFPresentationClock        *m_pClock;                  // Presentation clock.
};


class CVideoStream : public IMFStreamSink, public IMFMediaTypeHandler
{
    friend class CVideoSink;

public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID iid, void** ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IMFMediaEventGenerator
    IFACEMETHOD (BeginGetEvent)(IMFAsyncCallback* pCallback,IUnknown* punkState);
    IFACEMETHOD (EndGetEvent)(IMFAsyncResult* pResult, IMFMediaEvent** ppEvent);
    IFACEMETHOD (GetEvent)(DWORD dwFlags, IMFMediaEvent** ppEvent);
    IFACEMETHOD (QueueEvent)(MediaEventType met, REFGUID guidExtendedType, HRESULT hrStatus, const PROPVARIANT* pvValue);

    // IMFStreamSink
    IFACEMETHOD (GetMediaSink)(IMFMediaSink **ppMediaSink);
    IFACEMETHOD (GetIdentifier)(DWORD *pdwIdentifier);
    IFACEMETHOD (GetMediaTypeHandler)(IMFMediaTypeHandler **ppHandler);
    IFACEMETHOD (ProcessSample)(IMFSample *pSample);
    
    IFACEMETHOD (PlaceMarker)( 
        /* [in] */ MFSTREAMSINK_MARKER_TYPE eMarkerType,
        /* [in] */ const PROPVARIANT *pvarMarkerValue,
        /* [in] */ const PROPVARIANT *pvarContextValue);
    
    IFACEMETHOD (Flush)();

    // IMFMediaTypeHandler
    IFACEMETHOD (IsMediaTypeSupported)(IMFMediaType *pMediaType, IMFMediaType **ppMediaType);
    IFACEMETHOD (GetMediaTypeCount)(DWORD *pdwTypeCount);
    IFACEMETHOD (GetMediaTypeByIndex)(DWORD dwIndex, IMFMediaType **ppType);
    IFACEMETHOD (SetCurrentMediaType)(IMFMediaType *pMediaType);
    IFACEMETHOD (GetCurrentMediaType)(IMFMediaType **ppMediaType);
    IFACEMETHOD (GetMajorType)(GUID *pguidMajorType);


private:

    // State enum: Defines the current state of the stream.
    enum State
    {
        State_TypeNotSet = 0,    // No media type is set
        State_Ready,             // Media type is set, Start has never been called.
        State_Started,  
        State_Stopped,
        State_Paused,
        State_Finalized,

        State_Count = State_Finalized + 1    // Number of states
    };

    // StreamOperation: Defines various operations that can be performed on the stream.
    enum StreamOperation
    {
        OpSetMediaType = 0,
        OpStart,
        OpRestart,
        OpPause,
        OpStop,
        OpProcessSample,
        OpPlaceMarker,
        OpFinalize,

        Op_Count = OpFinalize + 1  // Number of operations
    };

    // CAsyncOperation:
    // Used to queue asynchronous operations. When we call MFPutWorkItem, we use this
    // object for the callback state (pState). Then, when the callback is invoked,
    // we can use the object to determine which asynchronous operation to perform.
 
    class CAsyncOperation : public IUnknown
    {
    public:
        CAsyncOperation(StreamOperation op);

        StreamOperation m_op;   // The operation to perform.  

        // IUnknown methods.
        STDMETHODIMP QueryInterface(REFIID iid, void** ppv);
        STDMETHODIMP_(ULONG) AddRef();
        STDMETHODIMP_(ULONG) Release();

    private:
        long    m_nRefCount;
        virtual ~CAsyncOperation();
    };

    
    // ValidStateMatrix: Defines a look-up table that says which operations
    // are valid from which states.
    static BOOL ValidStateMatrix[State_Count][Op_Count];


    CVideoStream();
    virtual ~CVideoStream();

    HRESULT Initialize(CVideoSink *pParent, IMFByteStream *pStream);

    HRESULT CheckShutdown() const
    {
        if (m_IsShutdown)
        {
            return MF_E_SHUTDOWN;
        }
        else
        {
            return S_OK;
        }
    }


    HRESULT     Start(MFTIME start);
    HRESULT     Restart();
    HRESULT     Stop();
    HRESULT     Pause();
    HRESULT     Finalize(IMFAsyncCallback *pCallback, IUnknown *punkState);
    HRESULT     Shutdown();

    HRESULT     ValidateOperation(StreamOperation op);

    HRESULT     QueueAsyncOperation(StreamOperation op);
    
    HRESULT     OnDispatchWorkItem(IMFAsyncResult* pAsyncResult);
    HRESULT     DispatchProcessSample(CAsyncOperation* pOp);
    HRESULT     DispatchFinalize(CAsyncOperation* pOp);

    HRESULT     ProcessSamplesFromQueue(FlushState bFlushData);
    HRESULT     WriteSampleToFile(IMFSample *pSample);
    HRESULT     SendMarkerEvent(IMarker *pMarker, FlushState bFlushData);


    long                        m_nRefCount;                // reference count
    CritSec                     m_critSec;                  // critical section for thread safety

    State                       m_state;
    BOOL                        m_IsShutdown;               // Flag to indicate if Shutdown() method was called.
    
    DWORD                       m_WorkQueueId;              // ID of the work queue for asynchronous operations.
    AsyncCallback<CVideoStream>   m_WorkQueueCB;              // Callback for the work queue.

    MFTIME                      m_StartTime;                // Presentation time when the clock started.
    DWORD                       m_cbDataWritten;            // How many bytes we have written so far.

    CVideoSink                    *m_pSink;                   // Parent media sink

    IMFMediaEventQueue          *m_pEventQueue;             // Event queue
    IMFByteStream               *m_pByteStream;             // Bytestream where we write the data.
    IMFMediaType                *m_pCurrentType;

    ComPtrList<IUnknown>        m_SampleQueue;              // Queue to hold samples and markers.
                                                            // Applies to: ProcessSample, PlaceMarker, BeginFinalize

    IMFAsyncResult              *m_pFinalizeResult;         // Result object for Finalize operation.

};




// Holds marker information for IMFStreamSink::PlaceMarker 

class CMarker : public IMarker
{
public:
    static HRESULT Create(
        MFSTREAMSINK_MARKER_TYPE eMarkerType,
        const PROPVARIANT* pvarMarkerValue,
        const PROPVARIANT* pvarContextValue,
        IMarker **ppMarker
        );

    // IUnknown methods.
    STDMETHODIMP QueryInterface(REFIID iid, void** ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    STDMETHODIMP GetMarkerType(MFSTREAMSINK_MARKER_TYPE *pType);
    STDMETHODIMP GetMarkerValue(PROPVARIANT *pvar);
    STDMETHODIMP GetContext(PROPVARIANT *pvar);

protected:
    MFSTREAMSINK_MARKER_TYPE m_eMarkerType;
    PROPVARIANT m_varMarkerValue;
    PROPVARIANT m_varContextValue;

private:
    long    m_nRefCount;

    CMarker(MFSTREAMSINK_MARKER_TYPE eMarkerType);
    virtual ~CMarker();
};


}
