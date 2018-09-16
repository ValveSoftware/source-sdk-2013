VERSION 5.00
Begin VB.Form Form1 
   BackColor       =   &H8000000B&
   BorderStyle     =   1  'Fixed Single
   Caption         =   "SAPI TAPI App"
   ClientHeight    =   4725
   ClientLeft      =   150
   ClientTop       =   720
   ClientWidth     =   4605
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   4725
   ScaleWidth      =   4605
   StartUpPosition =   3  'Windows Default
   Begin VB.TextBox StatusBox 
      Height          =   2655
      Left            =   0
      Locked          =   -1  'True
      MultiLine       =   -1  'True
      ScrollBars      =   2  'Vertical
      TabIndex        =   3
      Top             =   2040
      Width           =   4575
   End
   Begin VB.CommandButton AnswerBtn 
      Caption         =   "Answer"
      Height          =   495
      Left            =   840
      TabIndex        =   1
      Top             =   720
      Width           =   2895
   End
   Begin VB.ComboBox AddressTypesComBox 
      Height          =   315
      Left            =   840
      TabIndex        =   0
      Text            =   "Combo1"
      Top             =   120
      Width           =   2895
   End
   Begin VB.CommandButton DisconnectBtn 
      Caption         =   "Disconnect"
      Height          =   495
      Left            =   840
      TabIndex        =   2
      Top             =   1440
      Width           =   2895
   End
   Begin VB.Menu FileMenu 
      Caption         =   "File"
      Begin VB.Menu ExitMenu 
         Caption         =   "Exit"
      End
   End
   Begin VB.Menu AboutMenu 
      Caption         =   "About"
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
'=============================================================================
'
' This VB Speech Telephony App sample demonstrates how to use SAPI TTS and SR
' functionalities to speech enable the TAPI applications. The main objects used
' here are SAPI SpVoice, SpInprocRecognizer, SpMemoryStream TAPI object, and
' custom real time audio object STCUSTOMSTREAMLib.TTSStream and
' STCUSTOMSTREAMLib.ASRStream. The STCUSTOMSTREAMLib object is included in SAPI SDK.
' This object allows users to have the Internet calls between two computers and
' standard phone calls via the voice modem. This SAPI TAPI app uses the Internet call.
' However, users can enhance it to support the phone call by  supporting
' LINEADDRESSTYPE_PHONENUMBER in FindAddressIndex() subroutine and add
' LINEADDRESSTYPE_PHONENUMBER to the AddressTypesComBox ComboBox. For the detail
' information about how to add the support for phone calls via the voice modem,
' please refer to TAPI SDK.
'
'=============================================================================

Option Explicit

Dim WithEvents gObjVoice As SpVoice
Attribute gObjVoice.VB_VarHelpID = -1
Dim gObjRecognizer As SpInprocRecognizer
Dim WithEvents gObjRecoContext As SpInProcRecoContext
Attribute gObjRecoContext.VB_VarHelpID = -1
Dim RecoDictationGrammar As ISpeechRecoGrammar
Dim RecoCCGrammar As ISpeechRecoGrammar
Dim TopRule As ISpeechGrammarRule
Dim CmdRule As ISpeechGrammarRule

Dim WithEvents gObjTapiWithEvents As TAPI
Attribute gObjTapiWithEvents.VB_VarHelpID = -1
Dim gObjTapi As TAPI
Attribute gObjTapi.VB_VarHelpID = -1
Dim gobjAddress As ITAddress
Dim glRegistrationToken As Long
Dim gobjReceivedCallInfo As ITCallInfo
Dim gbSupportedCall As Boolean
Dim gobjCallControl As ITBasicCallControl

Dim TimeOut As Long
Dim gMemStream As SpMemoryStream
Dim gByeWave As New SpMemoryStream
Dim gChimeWaveStream As New SpMemoryStream

Dim gWriteSize As Long
Dim StreamSaveArray() As Long
Dim gIndex As Long

Dim objSRTerminal As ITTerminal
Dim objTTSTerminal As ITTerminal
Dim gNextCall As Boolean
Dim bNeedWaitForEndStreamEvent As Boolean
Dim bConnectionStatus As Boolean

Enum GRAMMARIDS
    GID_DICTATION = 1   'ID for the dictation grammar
    GID_CC = 3          'ID for the C&C grammar that's active when dictation is not
End Enum

Const TAPI3_ALL_TAPI_EVENTS = _
                            TE_ACDGROUP Or _
                            TE_ADDRESS Or _
                            TE_AGENT Or _
                            TE_AGENTHANDLER Or _
                            TE_AGENTSESSION Or _
                            TE_CALLHUB Or _
                            TE_CALLINFOCHANGE Or _
                            TE_CALLMEDIA Or _
                            TE_CALLNOTIFICATION Or _
                            TE_CALLSTATE Or _
                            TE_DIGITEVENT Or _
                            TE_GENERATEEVENT Or _
                            TE_PRIVATE Or _
                            TE_QOSEVENT Or _
                            TE_QUEUE Or _
                            TE_REQUEST Or _
                            TE_TAPIOBJECT
Const OPTTIMEOUT = &HFFFFF
Const LEAVEMESSAGEOPTION = &H1
Const CHECKMESSAGEOPTION = &H2
Const DISCONNECTED = &H4
Const EVENTENDSTREAM = &H5
Const VFW_E_NOT_COMMITTED = &H80040211 'DirectShow Error :Cannot allocate a sample when the allocator is not active

Function TimeIt(PauseTime, Key, ExKey, Optional altExKey = OPTTIMEOUT) As Boolean
   Dim Start, Finish, TotalTime

   Start = Timer   ' Get start time.
   Do While (Timer < Start + PauseTime) And (Key <> ExKey) And (Key <> altExKey)
      DoEvents     ' Yield to other processes.
   Loop
    
   If Key = ExKey Then
        TimeIt = True
   Else
        If Key = altExKey Then
            TimeIt = True
        Else
            TimeIt = False ' time out
        End If
   End If
End Function

Private Sub AboutMenu_Click()
    MsgBox "Copyright (c) 2001 Microsoft Corporation", vbOKOnly + vbInformation, "SAPI TAPI App"
End Sub

Private Sub AddressTypesComBox_Change()
    AddressTypesComBox.ListIndex = 0
    RegisterForReceiveCall
End Sub

Private Sub AddressTypesComBox_Click()
    AddressTypesComBox.ListIndex = 0
    RegisterForReceiveCall
End Sub

Private Sub LeaveMessage()

    Dim strMsg
    
    'Reset the indices
    gWriteSize = 0
    gIndex = 0
    ReDim StreamSaveArray(50)
    
    'Create a new stream
    Set gMemStream = New SpMemoryStream

    'Assume that the maxinum time is 20 seconds for leaving a message
    TimeOut = 0
    TimeIt 20, TimeOut, EVENTENDSTREAM, DISCONNECTED
    
End Sub

Private Sub AnswerBtn_Click()

    Dim strMsg

    'Disable the answer button
    AnswerBtn.Enabled = False
    gNextCall = True
    
    If gobjReceivedCallInfo Is Nothing Then
        strMsg = "There is no call to be answered!"
        StatusBox.SelText = vbCrLf & strMsg
        
        Exit Sub
    End If
        
    If Not (gobjReceivedCallInfo.CallState = CS_OFFERING) Then
        strMsg = "Cannot answer call that doesn't have offering state."
        StatusBox.SelText = vbCrLf & strMsg
       
        Exit Sub
    End If
    
    '
    'Not all addresses support ITTerminalSupport and ITStreamControl
    'therefore we need to be prepared for exceptions.
    '
    On Error Resume Next
    
      
    'Query ITBasicCallControl, the call control interface
    Set gobjCallControl = gobjReceivedCallInfo
    
    '
    'Createa two media streaming terminals: One for TTS and another for SR
    '
    'query ITTerminalSupport from Address object
    Dim objTerminalSupport As ITTerminalSupport
    Set objTerminalSupport = gobjAddress
    
    If Not (objTerminalSupport Is Nothing) Then
        
        Dim MediaStreamTerminalClsid As String
        
        MediaStreamTerminalClsid = "{E2F7AEF7-4971-11D1-A671-006097C9A2E8}"
        Set objTTSTerminal = objTerminalSupport.CreateTerminal( _
            MediaStreamTerminalClsid, TAPIMEDIATYPE_AUDIO, TD_CAPTURE)
        Set objSRTerminal = objTerminalSupport.CreateTerminal( _
            MediaStreamTerminalClsid, TAPIMEDIATYPE_AUDIO, TD_RENDER)
    
        
        'Release not needed objects
        Set objTerminalSupport = Nothing
        
        '
        'Select the terminals before answering - select them one by one;
        'we'll need the ITStreamControl interface for doing this.
        'Select each terminal on the corresponding stream.
        '
        
        'Note, if you are using a half-duplex voice modem, you only can select one
        'terminal at a time. Here, the app is designed for the Internet communications
        'so that the both terminals are selected
        
       
        Dim objStreamControl As ITStreamControl
        Set objStreamControl = gobjCallControl
        
        If Not (objStreamControl Is Nothing) Then
            Dim objITCollStreams As ITCollection
            
            Set objITCollStreams = objStreamControl.Streams
            
            Dim nIndex As Long, objCrtStream As ITStream
            
            For nIndex = 1 To objITCollStreams.Count
                Set objCrtStream = objITCollStreams.Item(nIndex)
                
                If objCrtStream.MediaType = TAPIMEDIATYPE_AUDIO Then
                    If objCrtStream.Direction = TD_CAPTURE Then
                        If Not (objTTSTerminal Is Nothing) Then
                            Call objCrtStream.SelectTerminal(objTTSTerminal)
                        End If
                    End If
                    If objCrtStream.Direction = TD_RENDER Then
                        If Not (objSRTerminal Is Nothing) Then
                           Call objCrtStream.SelectTerminal(objSRTerminal)
                        End If
                    End If
                End If
                
                Set objCrtStream = Nothing
            Next nIndex
        
            Set objITCollStreams = Nothing
            Set objStreamControl = Nothing
        End If

    End If
    
    On Error GoTo 0
   
    'Answer the call
    gobjCallControl.Answer
    
    '
    'Set the recognition state to active to get ready for recognition
    'Note: In SAPI 5.1, sapi turns off the recognition while the Read opration gets
    'any errors.
    gObjRecognizer.State = SRSActive
    
    
End Sub

Private Sub DisconnectBtn_Click()
                   
    'Set the disconnect flag
    TimeOut = DISCONNECTED
    
    If gobjReceivedCallInfo Is Nothing Then
        Dim strMsg As String
        strMsg = "There is no call to be disconnected."
        StatusBox.SelText = vbCrLf & strMsg
        
        Exit Sub
    End If
    
     If Not (gobjCallControl Is Nothing) Then
        gobjCallControl.Disconnect (DC_NORMAL)
    
        'release the call control interface
        Set gobjCallControl = Nothing
    End If
    
  
    'Deactive grammars
    RecoDictationGrammar.DictationSetState SGDSInactive
    RecoCCGrammar.CmdSetRuleState "TopLevelRule", SGDSInactive
        
    'Release terminals
    If Not (objTTSTerminal Is Nothing) Then
        Set objTTSTerminal = Nothing
    End If
    
    If Not (objSRTerminal Is Nothing) Then
        Set objSRTerminal = Nothing
    End If
    
    If bNeedWaitForEndStreamEvent Then
        'Wait about 30 seconds for the endstream event
        TimeIt 30, TimeOut, EVENTENDSTREAM
    End If
    
    'Disable the Disconnect and Answer button
    DisconnectBtn.Enabled = False
    AnswerBtn.Enabled = False
End Sub

Private Sub ExitMenu_Click()
   
    'Unload the form
    Unload Me

End Sub

Private Sub Form_Load()
    
    'Initialize Sapi
    Set gObjRecognizer = New SpInprocRecognizer
    Set gObjRecoContext = gObjRecognizer.CreateRecoContext
    
    'Obtain the voice from Reco Context
    Set gObjVoice = gObjRecoContext.Voice
    
    'Set interest events. Assume that we are only interested in the following four
    'SAPI SR events.
    gObjRecoContext.EventInterests = SRERecognition + SRESoundEnd + SREStreamEnd + _
                                    SREStreamStart + SRESoundEnd
    
    'Retain the audio so that we can Speak the audio later
    gObjRecoContext.RetainedAudio = SRAORetainAudio
    
    'Create and Load the grammars
    Set RecoDictationGrammar = gObjRecoContext.CreateGrammar(GID_DICTATION)
    Set RecoCCGrammar = gObjRecoContext.CreateGrammar(GID_CC)
       
    RecoDictationGrammar.DictationLoad vbNullString, SLOStatic
                
    'Build a dynamic Command and Control grammar
    Dim CommandState As ISpeechGrammarRuleState
      
    ' Add two rules. The top level rule will reference the command rule.
    ' User can choose the following commands: leave message, check message, leave, and check.
    Set TopRule = RecoCCGrammar.Rules.Add("TopLevelRule", SRATopLevel Or SRADynamic, 1)
    Set CmdRule = RecoCCGrammar.Rules.Add("CmdRule", SRADynamic, 2)
    
    Set CommandState = TopRule.AddState
  
    TopRule.InitialState.AddRuleTransition CommandState, CmdRule, 0, 0
    
    CmdRule.InitialState.AddWordTransition Nothing, "leave", " ", , "", 0, "", 0
    CmdRule.InitialState.AddWordTransition Nothing, "check", " ", , "", 1, 1, 1
    
    CommandState.AddWordTransition Nothing, "message", " ", , "", 1, "", 1
    
    RecoCCGrammar.Rules.Commit
        
    'Disable the Answer button. It will be enabled after a call is placed.
    AnswerBtn.Enabled = False
       
    'Disable the Disconnect button
    DisconnectBtn.Enabled = False
    
    'This application only supports the Internet call between two computers. You can use
    'AddressTypesComBox, to add more types of the connections, such as LINEADDRESSTYPE_PHONENUMBER
    'For example, if you want to support the phone calls, you need to add more code in
    'FindAddressIndex(). In addition, if you only have a half-duplex voice modem, then you
    'have to make some changes to ensure that you select one terminal at any time.
    
    AddressTypesComBox.AddItem ("H323 Calls")
    AddressTypesComBox.ItemData(AddressTypesComBox.NewIndex) = LINEADDRESSTYPE_DOMAINNAME
    
    AddressTypesComBox.ListIndex = 0
    
    'Load the tone wave file to the SpMemoryStream
    Dim WaveData As Variant
    WaveData = LoadResData("CHIMES", "CUSTOM")
    gChimeWaveStream.SetData WaveData
        
    'Load the good bye wave file
    Dim ByeWave As Variant
    ByeWave = LoadResData("BYEWAVE", "CUSTOM")
    gByeWave.SetData ByeWave
    
    'Init the flag. The flag will be set to true when a start stream event is received
    bNeedWaitForEndStreamEvent = False
    
    'There is no connection
    bConnectionStatus = False

End Sub

Private Sub RegisterForReceiveCall()
    Dim strMsg As String
    
    'Don't let the user do this in the middle of a call
    If Not (gobjReceivedCallInfo Is Nothing) Then
        StatusBox.SelText = vbCrLf & "Don't do this in the middle of a call."
        Exit Sub
    End If
    
    'Initialize Tapi
    Set gObjTapi = New TAPI
    
    'Initialize before calling any other tapi function
    Call gObjTapi.Initialize
    
    'Set the EventFilter to accept all defined tapi events
    gObjTapi.EventFilter = TAPI3_ALL_TAPI_EVENTS
        
    Set gObjTapiWithEvents = gObjTapi
    
    'Pick up the collection of addresses
    '
    Dim objcollAddress As ITCollection
    Set objcollAddress = gObjTapi.Addresses
    
    If (objcollAddress Is Nothing) Then
        StatusBox.SelText = vbCrLf & "Failed in query ITCollection"
        Exit Sub
    End If
        
    'Search address that supports the desired address type and media type
    '
    Dim nAddressIndex
    nAddressIndex = FindAddressIndex(objcollAddress)
    
    If nAddressIndex < 1 Or nAddressIndex > objcollAddress.Count Then
        
        strMsg = "Could not find an appropriate address for this address type!"
        StatusBox.SelText = vbCrLf & strMsg
        
        'Release the object
        Set objcollAddress = Nothing
        Exit Sub
    End If
    
    'If RegisterCallNotifications had been previously called,
    'unregister here, before registering for the new address
    'This is not a required step, a TAPI3 app can register for
    'receiving call notifications on more than one address in
    'the same time, but the app must be able to handle multiple
    'calls on multiple addresses.
    'This sample prefers to register on only one address at a time.
    If glRegistrationToken <> 0 Then
        Call gObjTapi.UnregisterNotifications(glRegistrationToken)
        glRegistrationToken = 0
    End If
        
    'pick up the "N"-th address - the address on which
    'you want to register for receiving calls
    Set gobjAddress = objcollAddress.Item(nAddressIndex)
    Set objcollAddress = Nothing    'no more needed, release
    
    'Register (specify) media types for which you want to receive calls;
    'only calls that have this media type will be offered to the app.
    'The media types must be passed to RegisterCallNotifications
    'bits in a "dword", which in VB is actually a "long".
    '
    Dim fOwner As Boolean, fMonitor As Boolean
    Dim lMediaTypes As Long, lCallbackInstance As Long
    
    'fOwner = True ensures that app receives incoming calls
    'and their call state events
    fOwner = True
    fMonitor = False
    lMediaTypes = TAPIMEDIATYPE_AUDIO
    lCallbackInstance = 1
    
    On Error Resume Next
    glRegistrationToken = gObjTapi.RegisterCallNotifications( _
        gobjAddress, fMonitor, fOwner, lMediaTypes, lCallbackInstance)
    If Err.Number <> 0 Then
        strMsg = "Registering for receiving calls failed." & vbCrLf & _
            "If you have a data modem, replace it with a voice modem. " & _
            "Quit the app and try again."
        
        StatusBox.SelText = vbCrLf & strMsg
        
        Exit Sub
    End If
    
    'From now on the app is able to receive calls made on the
    'specified address, with the specified media type
    
    StatusBox.SelText = vbCrLf & "Registration for calls succeeded. " & vbCrLf & "Waiting for a call..."
    
    Exit Sub
End Sub

'Search through all addresses that support  "audio".
'Return 0 if no address found. Otherwise return its index, which will be
'between 1 and Addresses.Count
Private Function FindAddressIndex(objCollAddresses As ITCollection) As Long
    
    Dim nSelectedType As Long
    Dim indexAddr As Long
    Dim objCrtAddress As ITAddress
    Dim lAddrTypes As Long
    Dim objMediaSupport As ITMediaSupport
    Dim objAddressCaps As ITAddressCapabilities
    Dim lMediaTypes As Long
    Dim bFound As Boolean
    
    '
    'Retrieve from combo box the type of the selected address type
    '
    nSelectedType = AddressTypesComBox.ItemData(AddressTypesComBox.ListIndex)
              
    '
    'search through all addresses the first one that matches this type
    'and also supports the media type "audio"
    '
    bFound = False
    
    For indexAddr = 1 To objCollAddresses.Count
        
        Set objCrtAddress = objCollAddresses.Item(indexAddr)
        
        Set objMediaSupport = objCrtAddress
        Set objAddressCaps = objCrtAddress

        lMediaTypes = objMediaSupport.MediaTypes

        '
        'Note: objMediaSupport.MediaTypes is a long that has
        'a bit set for each supported media type;
        'check if the bit for "audio" is set.
        '
        If lMediaTypes And TAPIMEDIATYPE_AUDIO Then

            lAddrTypes = objAddressCaps.AddressCapability(AC_ADDRESSTYPES)
            
            If nSelectedType = LINEADDRESSTYPE_DOMAINNAME Then
                'the user selected "h323 calls"
                'we must expect such an address to support
                'LINEADDRESSTYPE_DOMAINNAME; it's enough to check only
                'for this flag.
                If lAddrTypes And LINEADDRESSTYPE_DOMAINNAME Then
                    bFound = True
                End If
            End If
            
        End If
        
        Set objAddressCaps = Nothing
        Set objMediaSupport = Nothing
        Set objCrtAddress = Nothing
        
        If bFound = True Then Exit For
    Next indexAddr
    
    '
    'Return the index of the found address, or 0 if no address found
    '
    If bFound = True Then
        FindAddressIndex = indexAddr
    Else
        FindAddressIndex = 0
    End If
    Exit Function
End Function

Private Sub Form_Terminate()
    'Release all global objects
    
    Set gobjReceivedCallInfo = Nothing
    Set gobjAddress = Nothing
    Set gObjTapiWithEvents = Nothing
    If Not (gObjTapi Is Nothing) Then
        gObjTapi.Shutdown
    End If
    Set gObjTapi = Nothing
  
    'Deactivate the grammars and release them
    RecoDictationGrammar.DictationSetState SGDSInactive
    RecoCCGrammar.CmdSetRuleState vbNullString, SGDSInactive
    
    TopRule.Clear
    CmdRule.Clear

    Set RecoCCGrammar = Nothing
    Set RecoDictationGrammar = Nothing
    Set gObjRecoContext = Nothing
    Set gObjRecognizer = Nothing
        
    'Release the stream objects
    Set gMemStream = Nothing
    Set gByeWave = Nothing
    Set gChimeWaveStream = Nothing

End Sub

Private Sub Form_Unload(Cancel As Integer)

    If bConnectionStatus Then
        MsgBox "The application is busy. Cannot exit now. Please try to disconnect first"
        Cancel = True
    End If
        
End Sub

Private Sub gObjRecoContext_Recognition(ByVal StreamNumber As Long, ByVal StreamPosition As Variant, ByVal RecognitionType As SpeechLib.SpeechRecognitionType, ByVal Result As SpeechLib.ISpeechRecoResult)

    StatusBox.SelText = vbCrLf & "SR Recognition"
    Dim text As String

    Select Case Result.PhraseInfo.GrammarId
    
    Case GID_DICTATION
         ' Dictation events
         text = Result.PhraseInfo.GetText
         Dim SerializeResult As Variant
         Dim WriteSize As Long
        
         
         StatusBox.SelText = vbCrLf & text
         SerializeResult = Result.SaveToMemory
         WriteSize = gMemStream.Write(SerializeResult)
         
         If (gIndex > UBound(StreamSaveArray)) Then
             ReDim Preserve StreamSaveArray((UBound(StreamSaveArray) + 50))
         End If
         
         StreamSaveArray(gIndex) = WriteSize
         gWriteSize = gWriteSize + WriteSize
         gIndex = gIndex + 1
         
    Case GID_CC
        ' Command and Control events
        text = Result.PhraseInfo.GetText
        StatusBox.SelText = vbCrLf & text
        Select Case text
        
            Case "leave message", "leave"
                TimeOut = LEAVEMESSAGEOPTION
                RecoCCGrammar.CmdSetRuleState "TopLevelRule", SGDSInactive
                
            Case "check message", "check"
                TimeOut = CHECKMESSAGEOPTION
                RecoCCGrammar.CmdSetRuleState "TopLevelRule", SGDSInactive
        End Select
    End Select

End Sub

Private Sub gObjRecoContext_SoundEnd(ByVal StreamNumber As Long, ByVal StreamPosition As Variant)

    StatusBox.SelText = vbCrLf & "SR SoundEnd"
End Sub

Private Sub gObjRecoContext_SoundStart(ByVal StreamNumber As Long, ByVal StreamPosition As Variant)

    StatusBox.SelText = vbCrLf & "SR SoundStart"
End Sub

Private Sub gObjRecoContext_StartStream(ByVal StreamNumber As Long, ByVal StreamPosition As Variant)
    bNeedWaitForEndStreamEvent = True
    StatusBox.SelText = vbCrLf & "SR StartStream"
End Sub

Private Sub gObjRecoContext_EndStream(ByVal StreamNumber As Long, ByVal StreamPosition As Variant, ByVal StreamReleased As Boolean)
    TimeOut = EVENTENDSTREAM
    bNeedWaitForEndStreamEvent = False
    StatusBox.SelText = vbCrLf & "SR EndStream"
End Sub

Private Sub gobjTapiWithEvents_Event(ByVal TapiEvent As TAPI3Lib.TAPI_EVENT, ByVal pEvent As Object)
   
    Dim strMsg
    
    If TapiEvent = TE_CALLNOTIFICATION Then
        
        StatusBox.SelText = vbCrLf & "TAPI: TE_CALLNOTIFICATION"
        
        'In the case of TE_CALLNOTIFICATION, pEvent contains an ITCallNotficationEvent
        'interface, so query for the interface
        
        Dim objCallNotificationEvent As ITCallNotificationEvent
        Set objCallNotificationEvent = pEvent
        
        'Decide if we can take this call: this  app only
        'supports one call at a time, so if it already has a call,
        'it will reject any other call that arrives in the same time.
        '
        gbSupportedCall = True
        
        If Not (gobjReceivedCallInfo Is Nothing) Then
            
            gbSupportedCall = False
            
            strMsg = "This app doesn't support a second call. "
            strMsg = strMsg & "Unsupported second call will be rejected!"
            
            StatusBox.SelText = vbCrLf & strMsg
            
            'Note: objCallNotificationEvent.Call actually contains an
            'ITCallInfo interface, but by assigning it to an ITBasicCallControl
            'interface, we actually query for the interface "ITBasicCallControl"
            Dim objReceivedCallControl As ITBasicCallControl
            Set objReceivedCallControl = objCallNotificationEvent.Call
            
            'Reject the not supported call by calling Disconnect
            'Note: this second call will arrive only if the tsp (tapi service provider)
            'supports more than 1 call per address.
            Dim code As DISCONNECT_CODE
            code = DC_REJECTED
            objReceivedCallControl.Disconnect (code)
            
            'release all objects that are not needed any longer
            Set objReceivedCallControl = Nothing
            Set objCallNotificationEvent = Nothing
            
            Exit Sub
            
        End If
        
        'Query ITCallInfo interface for the new call, and store it
        Set gobjReceivedCallInfo = objCallNotificationEvent.Call
        
        'Reenable the button
        AnswerBtn.Enabled = True
    
        Set objCallNotificationEvent = Nothing
        
        Exit Sub
    End If
    
    If TapiEvent = TE_CALLSTATE Then
        
        StatusBox.SelText = vbCrLf & "TAPI: TE_CALLSTATE"
        
        'For this type of event, the object pEvent must be
        'queried for its ITCallStateEvent interface
        Dim objCallStateEvent As ITCallStateEvent
        Set objCallStateEvent = pEvent
        
        Dim State As CALL_STATE
        Dim objEventCallInfo As ITCallInfo
        
        'Extract the call object from pEvent (from its
        'ITCallStateEvent interface)
        Set objEventCallInfo = objCallStateEvent.Call
        State = objCallStateEvent.State
        
        If objEventCallInfo Is gobjReceivedCallInfo Then
        
            'Display the all status
            DisplayCallState (State)
            
            If State = CS_DISCONNECTED Then
                'After call is disconnected, release the object
                Set gobjReceivedCallInfo = Nothing
            End If
        
        End If
        
        Set objEventCallInfo = Nothing
        Set objCallStateEvent = Nothing
        
        Exit Sub
    End If
    
    If TapiEvent = TE_CALLMEDIA Then
        'Handle the call
        HandleTheCall pEvent
     
    End If
    
    Exit Sub
End Sub

Private Sub HandleTheCall(pEvent As Object)
    Dim MediaEvent As ITCallMediaEvent
    
    Set MediaEvent = pEvent
    
    'Handle one call at a time when the connection is established and
    ' the media stream is active
    If (CME_STREAM_ACTIVE = MediaEvent.Event) And (gNextCall) And (bConnectionStatus) Then
     
        gNextCall = False
        Dim CustomStream As New SpCustomStream
        Dim SapiTapiTTSStream As STCUSTOMSTREAMLib.TTSStream
        Dim SapiTapiSRStream As STCUSTOMSTREAMLib.ASRStream
    
             
        ''''''''''''''
        ''TTS
        ''''''''''''''
        'Create the audio stream object
        Set SapiTapiTTSStream = New STCUSTOMSTREAMLib.TTSStream
        
        'Initialize the stream object
        SapiTapiTTSStream.InitTTSCaptureStream objTTSTerminal
    
        'Set the audio stream object as the BaseStream
        Set CustomStream.BaseStream = SapiTapiTTSStream
       
        'Prevent the format from being changes
        gObjVoice.AllowAudioOutputFormatChangesOnNextSet = False
        
        'Set up the audio output
        Set gObjVoice.AudioOutputStream = CustomStream
        
        'Release the objects
        Set SapiTapiTTSStream = Nothing
        Set CustomStream = Nothing
        
        ''''''''''''''
        ''SR
        ''''''''''''''
        'Create a SAPI CustomStream object
        Set CustomStream = New SpCustomStream
        
        'Create the audio stream object
        Set SapiTapiSRStream = New STCUSTOMSTREAMLib.ASRStream
        
        'Initialize the stream object
        SapiTapiSRStream.InitSRRenderStream objSRTerminal
    
        'Set the audio stream object as the BaseStream
        Set CustomStream.BaseStream = SapiTapiSRStream
        
        'Prevent the format from being changes
        gObjRecognizer.AllowAudioInputFormatChangesOnNextSet = False
        
        'Set up the audio input
        Set gObjRecognizer.AudioInputStream = CustomStream
        
        'Release the object
        Set CustomStream = Nothing
        
        'Wait 3 seconds for the media streaming to be active. In most cases, this
        'is unnecessary. Somehow on some machines, the media stream is still
        'inactive although we have CME_STREAM_ACTIVE event.
        
        StatusBox.SelText = vbCrLf & "Waiting for the media streaming to be active ..."
        Dim j
        TimeOut = 0
        For j = 0 To 3
          TimeIt 1, TimeOut, 1, DISCONNECTED
          StatusBox.SelText = "."
        Next j
   
        'If users click the disconnect button, exit the app
        If (TimeOut = DISCONNECTED) Then
            Set SapiTapiSRStream = Nothing
            Exit Sub
        End If
        
        
        On Error GoTo ErrorHandler
        'greetings
        gObjVoice.Speak "Welcome to the Speech and Telephony API app."
        gObjVoice.Speak "Please select the following <SILENCE MSEC='10'/><EMPH>two</EMPH><SILENCE MSEC='10'/> options: Leave message or Check message", SVSFIsXML

        'use Command and Control for the option menu
        RecoCCGrammar.CmdSetRuleState "TopLevelRule", SGDSActive

        'the maxinum time for selecting a menu is 10 seconds
        TimeOut = 0
        TimeIt 10, TimeOut, LEAVEMESSAGEOPTION, CHECKMESSAGEOPTION
        RecoCCGrammar.CmdSetRuleState "TopLevelRule", SGDSInactive
 
        If (TimeOut = LEAVEMESSAGEOPTION) Then
            'User selects "leave a message menu"
            'Reset the timeout flag
            TimeOut = 0
        
            'Wait for the endstream event
            TimeIt 30, TimeOut, EVENTENDSTREAM, DISCONNECTED
   
            gObjVoice.Speak "Please leave the message after the tone"
            gChimeWaveStream.Seek 0, SSSPTRelativeToStart
            gObjVoice.SpeakStream gChimeWaveStream
  
            RecoDictationGrammar.DictationSetState SGDSActive
            LeaveMessage
            RecoDictationGrammar.DictationSetState SGDSInactive
            
            'Release the object
            Set SapiTapiSRStream = Nothing
            
            'Wait for the endstream event
            TimeIt 30, TimeOut, EVENTENDSTREAM, DISCONNECTED
                    
            'Play back the audio if any
            If (gWriteSize <> 0) Then
                StatusBox.SelText = vbCrLf & "Playing the message"
                gMemStream.Seek 0, SSSPTRelativeToStart
                Dim resultGet As Variant, length As Long
                Dim RecoResultGet As ISpeechRecoResult

                gObjVoice.Speak "Following is your message"

                Dim i
                For i = 0 To gIndex - 1

                    length = gMemStream.Read(resultGet, StreamSaveArray(i))
                    Set RecoResultGet = gObjRecoContext.CreateResultFromMemory(resultGet)
                    DoEvents
                    RecoResultGet.SpeakAudio
                    Set RecoResultGet = Nothing
                Next i
                gObjVoice.Speak "end of your message"
                
            End If
            
        Else
            If (TimeOut = CHECKMESSAGEOPTION) Then
                StatusBox.SelText = vbCrLf & "Playing the message"
                'Reset the timeout flag
                TimeOut = 0
        
                'Wait for the endstream event
                TimeIt 30, TimeOut, EVENTENDSTREAM, DISCONNECTED
   
                If (gWriteSize <> 0) Then
                    gMemStream.Seek 0, SSSPTRelativeToStart
                    gObjVoice.Speak "The following is your message"
                
                    For i = 0 To gIndex - 1

                        length = gMemStream.Read(resultGet, StreamSaveArray(i))
                        Set RecoResultGet = gObjRecoContext.CreateResultFromMemory(resultGet)
                        DoEvents
                        RecoResultGet.SpeakAudio
                        Set RecoResultGet = Nothing
                    Next i
                    gObjVoice.Speak "end of your message"
                Else
                    gObjVoice.Speak "You do not have any messages"
                    
                End If
            Else
                gObjVoice.Speak "You did not select anything"
                
            End If ' check message
        End If 'leave message
        gObjVoice.Speak "good bye!"
        gByeWave.Seek 0, SSSPTRelativeToStart
        gObjVoice.SpeakStream gByeWave
        
        'Release the object
        If Not (SapiTapiSRStream Is Nothing) Then
            Set SapiTapiSRStream = Nothing
        End If
        
        DisconnectBtn_Click
       
    End If ' media stream active
    Exit Sub
    
ErrorHandler:
    If Err.Number = VFW_E_NOT_COMMITTED Then
        StatusBox.SelText = vbCrLf & "The connection was lost. Disconnecting ..."
        DisconnectBtn_Click
    Else
        MsgBox "Error occurs in HandleTheCall: " & Err.Description & vbCrLf & " " & Err.Number
        Resume Next
    End If
    'Release the object
    If Not (SapiTapiSRStream Is Nothing) Then
        Set SapiTapiSRStream = Nothing
    End If
End Sub

Private Sub DisplayCallState(State As CALL_STATE)
    Dim strMsg As String
    
    Select Case State
        Case CS_CONNECTED
            bConnectionStatus = True
            strMsg = "call state: CS_CONNECTED"
            StatusBox.SelText = vbCrLf & strMsg
            
            'Enable the Disconnect button
            DisconnectBtn.Enabled = True
        Case CS_DISCONNECTED
            DisconnectBtn_Click
            bConnectionStatus = False
            strMsg = "call state: CS_DISCONNECTED" & vbCrLf
            strMsg = strMsg & "New incoming calls will be accepted. "
            
            StatusBox.SelText = vbCrLf & strMsg
        Case CS_HOLD
            
            StatusBox.SelText = vbCrLf & "call state: CS_HOLD"
        Case CS_IDLE
            
            StatusBox.SelText = vbCrLf & "call state: CS_IDLE"
        
        Case CS_INPROGRESS
            
            StatusBox.SelText = vbCrLf & "call state: CS_INPROGRESS"
        
        Case CS_OFFERING
            If gbSupportedCall = True Then
                strMsg = "call state: CS_OFFERING"
                Dim RemoteMachine As String
                
                '.CallInfoString fails sometimes on some networks, so we ignore the error in
                'getting the remote machine name
                On Error Resume Next
                'get caller's machine name
                RemoteMachine = gobjReceivedCallInfo.CallInfoString(CIS_CALLERIDNAME)
                On Error GoTo 0
                
                strMsg = strMsg & vbCrLf & "A call "
                If Not (RemoteMachine = "") Then
                     'Some machine names contain the line feed. So we remove it here
                     strMsg = strMsg & "from ' "
                     Dim str
                     For Each str In Split(RemoteMachine, vbLf)
                         strMsg = strMsg & str & " "
                     Next str
                     strMsg = strMsg & "' "
                End If
                strMsg = strMsg & "has been received. You can answer it by clicking the Answer button."
                StatusBox.SelText = vbCrLf & strMsg
            End If
        
        Case CS_QUEUED
            StatusBox.SelText = vbCrLf & "call state: CS_QUEUED"
            
        Case Else
            StatusBox.SelText = vbCrLf & "call state: unknown!!"
            
    End Select
     
End Sub

