VERSION 5.00
Begin VB.Form Form1 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "VB Outgoing Call"
   ClientHeight    =   3555
   ClientLeft      =   150
   ClientTop       =   720
   ClientWidth     =   6105
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   3555
   ScaleWidth      =   6105
   StartUpPosition =   3  'Windows Default
   Begin VB.TextBox Text1 
      Height          =   1815
      Left            =   240
      MultiLine       =   -1  'True
      ScrollBars      =   2  'Vertical
      TabIndex        =   1
      Text            =   "VBOutgoingcall.frx":0000
      Top             =   840
      Width           =   4455
   End
   Begin VB.ComboBox DestAddressCombo 
      Height          =   315
      Left            =   1320
      TabIndex        =   0
      Top             =   240
      Width           =   3375
   End
   Begin VB.CommandButton DisconnectBtn 
      Caption         =   "Disconnect"
      Height          =   450
      Left            =   4920
      TabIndex        =   4
      Top             =   1320
      Width           =   1000
   End
   Begin VB.CommandButton CallBtn 
      Caption         =   "Dial"
      Height          =   450
      Left            =   4920
      TabIndex        =   2
      Top             =   240
      Width           =   1000
   End
   Begin VB.Label Label1 
      Caption         =   "Internet Call"
      Height          =   255
      Left            =   240
      TabIndex        =   6
      Top             =   240
      Width           =   975
   End
   Begin VB.Label TitleLbl 
      Caption         =   "Call Status:"
      Height          =   255
      Left            =   120
      TabIndex        =   5
      Top             =   3000
      Width           =   855
   End
   Begin VB.Label StatusLbl 
      Height          =   375
      Left            =   1080
      TabIndex        =   3
      Top             =   3000
      Width           =   3495
   End
   Begin VB.Menu menuFile 
      Caption         =   "File"
      Begin VB.Menu menuFileExit 
         Caption         =   "Exit"
      End
   End
   Begin VB.Menu menuHelp 
      Caption         =   "Help"
      Begin VB.Menu menuAbout 
         Caption         =   "About"
      End
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
'=============================================================================
'
' This VB Speech Telephony App sample demonstrates how to use SAPI TTS
' functionalities to speech enable the TAPI apps. The main objects used
' here are SAPI SpVoice, TAPI object, and custom real time audio object
' STCUSTOMSTREAMLib.TTSStream
'
'=============================================================================
Option Explicit

Dim WithEvents gObjTapiWithEvents As TAPI
Attribute gObjTapiWithEvents.VB_VarHelpID = -1
Dim WithEvents gObjVoice As SpVoice
Attribute gObjVoice.VB_VarHelpID = -1
Dim gObjTapi As TAPI
Dim gobjAddress As ITAddress
Dim objCallControl As ITBasicCallControl
Dim AddressTypeSelected As Long
Dim MemStream As SpMemoryStream
Dim bConnectionStatus As Boolean
Const VFW_E_NOT_COMMITTED = &H80040211 'DirectShow Error :Cannot allocate a sample when the allocator is not active

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
Private Sub menuAbout_Click()
    MsgBox "VB Outgoing Call App" & vbCrLf & vbCrLf & "Copyright (c) 2001 Microsoft Corporation. All rights reserved.", _
        vbOKOnly Or vbInformation, "VB Outgoing Call"
End Sub

Private Sub menuFileExit_Click()
    Unload Me
End Sub

Private Sub CallBtn_Click()
    StatusLbl.Caption = _
    "Dial ..."
    StatusLbl.Refresh
    DisconnectBtn.Enabled = True
    Call MakeTheCall
End Sub

Private Sub DisconnectBtn_Click()
   
    Dim strMsg As String
    
    If objCallControl Is Nothing Then
        strMsg = "Already disconnected."
        StatusLbl.Caption = strMsg
        StatusLbl.Refresh
    Else
        objCallControl.Disconnect (DC_NORMAL)
        
        'Since the call has been disconnected, disable the Disconnect button
        DisconnectBtn.Enabled = False
    End If
       
End Sub

Private Sub Form_Load()
    Dim strMsg As String
    Dim objcollAddress As ITCollection
    Dim nAddressIndex As Long

    'Initialize Sapi
    Set gObjVoice = New SpVoice
    
    'Create a Tapi object
    Set gObjTapi = New TAPI
    
    'Initialize TAPI. this must be called before
    'any other tapi functions are called.
    Call gObjTapi.Initialize
    
    'set the EventFilter to accept all defined tapi events
    gObjTapi.EventFilter = TAPI3_ALL_TAPI_EVENTS
        
    Set gObjTapiWithEvents = gObjTapi
   
    Call IpAddressOpt
    
    'Load a good bye wave file and write it to the SpMemoryStream
    Dim WaveData As Variant
    WaveData = LoadResData("WAVEFILE", "CUSTOM")
    
    Set MemStream = New SpMemoryStream
    MemStream.SetData WaveData
           
    'Disable the button right now. It will be enabled after the connection
    DisconnectBtn.Enabled = False
    
    'There is no connection right now
    bConnectionStatus = False
End Sub

Private Sub Form_Unload(Cancel As Integer)

    If bConnectionStatus Then
        MsgBox "The application is busy. Cannot exit now. Please try to disconnect first"
        'do not exit the app
        Cancel = True
    Else
        'shut down tapi and release global objects
        If Not (gObjTapi Is Nothing) Then
            gObjTapi.Shutdown
        End If
        Set objCallControl = Nothing
        Set gobjAddress = Nothing
        Set gObjTapi = Nothing
        Set MemStream = Nothing
    End If
End Sub

Private Sub FindAnAddress(AddressTypeSelected As Long)
    Dim Address As ITAddress
    Dim AddressCaps As ITAddressCapabilities
    Dim lType As Long
    Dim MediaSupport As ITMediaSupport
      
    For Each Address In gObjTapi.Addresses
    
        'query for ITAddressCapabilities
        Set AddressCaps = Address
        lType = AddressCaps.AddressCapability(AC_ADDRESSTYPES)
        Set AddressCaps = Nothing
        
        'is the type we are looking for?
        If lType And AddressTypeSelected Then
           If AddressSupportsMediaType(Address, TAPIMEDIATYPE_AUDIO) Then
                If Len(Address.AddressName) <> 0 Then
                    'save to global variable
                    Set gobjAddress = Address
                    Exit For
                End If
            End If
        End If
        
        Set Address = Nothing
    
    Next
  
End Sub

Private Function AddressSupportsMediaType(Address As ITAddress, lType As Long) As Boolean

    Dim bType As Boolean
    Dim pMediaSupport As ITMediaSupport
    bType = False
    
    'Check whether the service provider associated with the current address
    'supports the media type, lType

    Set pMediaSupport = Address
    If pMediaSupport Is Nothing Then
        bType = False
    Else
        bType = pMediaSupport.QueryMediaType(lType)
    End If
    
    Set pMediaSupport = Nothing
    AddressSupportsMediaType = bType
End Function

Private Sub SelectTerminalsOnCall()
    Dim objStreamControl As ITStreamControl
    Set objStreamControl = objCallControl
    
    If Not (objStreamControl Is Nothing) Then
        Dim objITCollStreams As ITCollection
        Dim nIndex As Long, objCrtStream As ITStream
        Dim objTerminalSupport As ITTerminalSupport
        
        Set objITCollStreams = objStreamControl.Streams
        Set objTerminalSupport = gobjAddress
        
        'Create a media streaming terminal and select the capture stream for the SAPI/TTS
        'audio output
        Dim objTerminal As ITTerminal
        Dim MediaStreamTerminalClsid As String
        MediaStreamTerminalClsid = "{E2F7AEF7-4971-11D1-A671-006097C9A2E8}"
        
        For nIndex = 1 To objITCollStreams.Count
            Set objCrtStream = objITCollStreams.Item(nIndex)
            
        
            If (objCrtStream.Direction = TD_CAPTURE) Then
                Set objTerminal = objTerminalSupport.CreateTerminal( _
                    MediaStreamTerminalClsid, objCrtStream.MediaType, objCrtStream.Direction)
            
                Call objCrtStream.SelectTerminal(objTerminal)
            End If
            Set objCrtStream = Nothing
        Next nIndex
        
        ''''''''''''''
        ''USE SAPI TTS
        ''''''''''''''
        Dim CustomStream As New SpCustomStream
        Dim SapiTapiTTSStream As STCUSTOMSTREAMLib.TTSStream
    
        'Create the TTSStream object
        Set SapiTapiTTSStream = New STCUSTOMSTREAMLib.TTSStream
        
        'Initialize the TTSStream object
        SapiTapiTTSStream.InitTTSCaptureStream objTerminal
    
        'Set the TTSStream object as a BaseStream in the SAPI CustomStream object
        Set CustomStream.BaseStream = SapiTapiTTSStream
       
        'Use the current format and prevent the SAPI object from changing it
        gObjVoice.AllowAudioOutputFormatChangesOnNextSet = False
        
        'Set the audio output to the SAPI CustomStream
        Set gObjVoice.AudioOutputStream = CustomStream
        
        'release not needed objects
        Set SapiTapiTTSStream = Nothing
        Set CustomStream = Nothing
        Set objTerminalSupport = Nothing
        Set objITCollStreams = Nothing
        Set objStreamControl = Nothing
    End If
End Sub

Private Sub MakeTheCall()
    Dim lMediaType As Long
    
    If (AddressSupportsMediaType(gobjAddress, TAPIMEDIATYPE_AUDIO)) Then
    
       lMediaType = TAPIMEDIATYPE_AUDIO + lMediaType
    End If
    
    Set objCallControl = gobjAddress.CreateCall(DestAddressCombo.Text, _
                                        AddressTypeSelected, lMediaType)
                                        
                                        
    Call SelectTerminalsOnCall
 
  
    'connect
    On Error GoTo ErrHandler
    objCallControl.Connect False
    On Error GoTo 0

ErrHandler:
    If Err.Number = -2147221492 Then
        MsgBox "Connection failed. Is the domain name or the IP Address correct?"
        StatusLbl.Caption = "Connection failed."
        DisconnectBtn.Enabled = False
    End If
End Sub

Private Sub gobjTapiWithEvents_Event(ByVal TapiEvent As TAPI3Lib.TAPI_EVENT, ByVal pEvent As Object)
   
    Dim strMsg

    Select Case TapiEvent
        Case TE_CALLNOTIFICATION

    Case TE_CALLSTATE

        'for this type of event, the object pEvent must be
        'queried for its ITCallStateEvent interface
        Dim objCallStateEvent As ITCallStateEvent
        Set objCallStateEvent = pEvent

        Dim State As CALL_STATE
        State = objCallStateEvent.State

        DisplayCallState (State)
        Set objCallStateEvent = Nothing

    Case TE_CALLMEDIA
        Dim MediaEvent As ITCallMediaEvent
        Set MediaEvent = pEvent
        
        Select Case MediaEvent.Event
        Case CME_STREAM_ACTIVE
                 
            On Error GoTo ErrorHandler
            'prompt greetings
            If (Text1.Text = "") Then
                gObjVoice.Speak "Hello"
                gObjVoice.Speak "Welcome to the Speech and Telephony API app. Have a nice day! Bye now", SVSFlagsAsync
                
            Else
                gObjVoice.Speak Text1.Text, SVSFlagsAsync
            End If
            
            'Wait until the speak completes
            Do
                DoEvents
            Loop Until gObjVoice.WaitUntilDone(1) = True
            
            
            MemStream.Seek 0, SSSPTRelativeToStart
            gObjVoice.SpeakStream MemStream
           
            DisconnectBtn_Click
        End Select
    End Select
    Exit Sub
    
ErrorHandler:
    If Err.Number = VFW_E_NOT_COMMITTED Then
        StatusLbl.Caption = "The connection was lost. Disconnecting ..."
        StatusLbl.Refresh
        DisconnectBtn_Click
    Else
        MsgBox "Error occurs: " & Err.Description & Err.Number
    End If
End Sub

Private Sub DisplayCallState(State As CALL_STATE)
    Dim strMsg As String
    
    'Display the current call status
    Select Case State
        Case CS_CONNECTED
            'The app is connected so we set the connection status to true
            bConnectionStatus = True
            StatusLbl.Caption = "CS_CONNECTED"
            
        Case CS_DISCONNECTED
            DisconnectBtn_Click
            'The app is disconnected so we set the connection status to false
            bConnectionStatus = False
            StatusLbl.Caption = "CS_DISCONNECTED"
            
        Case CS_HOLD
            StatusLbl.Caption = " CS_HOLD"
            
        Case CS_IDLE
            StatusLbl.Caption = "CS_IDLE"
        
        Case CS_INPROGRESS
            StatusLbl.Caption = "CS_INPROGRESS"
        
        Case CS_OFFERING
            StatusLbl.Caption = "CS_OFFERING"
          
        Case CS_QUEUED
            StatusLbl.Caption = "CS_QUEUED"
        
        Case Else
            StatusLbl.Caption = "Unknown!!"
    End Select
    
    StatusLbl.Refresh
End Sub

Private Sub IpAddressOpt()
    AddressTypeSelected = LINEADDRESSTYPE_DOMAINNAME
    FindAnAddress AddressTypeSelected
  
    'The app only supports the Internet call. You can add your interest IP
    'addresses or machine names here programmatically or simply add them in the
    'Properties of the DestAddressCombo ComBobox.

    DestAddressCombo.Text = "<Machine Name> or IP address"
    If Not (gobjAddress Is Nothing) Then
        StatusLbl.Caption = "Using " & gobjAddress.AddressName
    End If
    StatusLbl.Refresh
End Sub

