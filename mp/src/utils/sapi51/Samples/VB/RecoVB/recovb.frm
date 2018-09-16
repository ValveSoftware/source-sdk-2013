VERSION 5.00
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.2#0"; "comdlg32.ocx"
Object = "{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}#2.0#0"; "mscomctl.ocx"
Begin VB.Form Form1 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "Basic Speech Recognition"
   ClientHeight    =   9195
   ClientLeft      =   150
   ClientTop       =   720
   ClientWidth     =   9435
   ForeColor       =   &H000000FF&
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   613
   ScaleMode       =   3  'Pixel
   ScaleWidth      =   629
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton PlayAudio 
      Caption         =   "Play Audio"
      Height          =   375
      Left            =   4320
      TabIndex        =   39
      Top             =   8760
      Width           =   1815
   End
   Begin VB.CheckBox RetainAudio 
      Caption         =   "Retain Audio"
      Height          =   255
      Left            =   6240
      TabIndex        =   38
      Top             =   8880
      Width           =   1215
   End
   Begin VB.Frame Frame4 
      Caption         =   "Engine Creation"
      Height          =   975
      Left            =   6240
      TabIndex        =   36
      Top             =   1920
      Width           =   3135
      Begin VB.OptionButton SharedRC 
         Caption         =   "Shared"
         Height          =   255
         Left            =   120
         TabIndex        =   3
         Top             =   240
         Value           =   -1  'True
         Width           =   1455
      End
      Begin VB.OptionButton Inproc 
         Caption         =   "Inproc"
         Height          =   255
         Left            =   120
         TabIndex        =   37
         Top             =   600
         Width           =   1455
      End
   End
   Begin VB.CheckBox ActivateMic 
      Caption         =   "Activate Mic"
      Height          =   255
      Left            =   6240
      TabIndex        =   35
      Top             =   480
      Value           =   1  'Checked
      Width           =   1335
   End
   Begin VB.CommandButton ClearTree 
      Caption         =   "Clear Tree View"
      Height          =   375
      Left            =   2220
      TabIndex        =   27
      Top             =   8760
      Width           =   1815
   End
   Begin VB.ComboBox SREngines 
      Enabled         =   0   'False
      Height          =   315
      Left            =   6240
      TabIndex        =   4
      Text            =   "SREngines"
      Top             =   3120
      Width           =   3135
   End
   Begin MSComctlLib.TreeView TreeView1 
      Height          =   6375
      Left            =   120
      TabIndex        =   34
      Top             =   0
      Width           =   6015
      _ExtentX        =   10610
      _ExtentY        =   11245
      _Version        =   393217
      LineStyle       =   1
      Style           =   7
      Appearance      =   1
   End
   Begin VB.CommandButton ExitBtn 
      Caption         =   "Exit"
      Height          =   375
      Left            =   7560
      TabIndex        =   28
      Top             =   8760
      Width           =   1815
   End
   Begin VB.CommandButton ClearEvents 
      Caption         =   "Clear Event List"
      Height          =   375
      Left            =   120
      TabIndex        =   26
      Top             =   8760
      Width           =   1815
   End
   Begin VB.Frame Frame3 
      Caption         =   "Event Interests"
      Height          =   2895
      Left            =   6240
      TabIndex        =   33
      Top             =   5760
      Width           =   3135
      Begin VB.CheckBox StreamStart 
         Caption         =   "Stream Start"
         Height          =   255
         Left            =   1560
         TabIndex        =   24
         Top             =   2520
         Width           =   1215
      End
      Begin VB.CheckBox StreamEnd 
         Caption         =   "Stream End"
         Height          =   255
         Left            =   1560
         TabIndex        =   23
         Top             =   2280
         Width           =   1215
      End
      Begin VB.CheckBox StateChange 
         Caption         =   "State Change"
         Height          =   255
         Left            =   1560
         TabIndex        =   22
         Top             =   2040
         Width           =   1455
      End
      Begin VB.CheckBox SoundStart 
         Caption         =   "Sound Start"
         Height          =   255
         Left            =   1560
         TabIndex        =   21
         Top             =   1800
         Width           =   1215
      End
      Begin VB.CheckBox SoundEnd 
         Caption         =   "Sound End"
         Height          =   255
         Left            =   1560
         TabIndex        =   20
         Top             =   1560
         Width           =   1215
      End
      Begin VB.CheckBox RequestUI 
         Caption         =   "Request UI"
         Height          =   255
         Left            =   1560
         TabIndex        =   19
         Top             =   1320
         Width           =   1215
      End
      Begin VB.CheckBox Reco 
         Caption         =   "Recognition"
         Height          =   255
         Left            =   1560
         TabIndex        =   17
         Top             =   720
         Width           =   1215
      End
      Begin VB.CheckBox PropertyStringChange 
         Caption         =   "Property String Change"
         Height          =   375
         Left            =   1560
         TabIndex        =   16
         Top             =   360
         Width           =   1335
      End
      Begin VB.CheckBox PhraseStart 
         Caption         =   "Phrase Start"
         Height          =   255
         Left            =   120
         TabIndex        =   13
         Top             =   1920
         Width           =   1215
      End
      Begin VB.CheckBox Interference 
         Caption         =   "Interference"
         Height          =   255
         Left            =   120
         TabIndex        =   12
         Top             =   1680
         Width           =   1215
      End
      Begin VB.CheckBox FalseReco 
         Caption         =   "False Recognition"
         Height          =   375
         Left            =   120
         TabIndex        =   10
         Top             =   1080
         Width           =   1335
      End
      Begin VB.CheckBox Bookmark 
         Caption         =   "Bookmark"
         Height          =   255
         Left            =   120
         TabIndex        =   9
         Top             =   840
         Width           =   1215
      End
      Begin VB.CheckBox Adaption 
         Caption         =   "Adaptation"
         Height          =   255
         Left            =   120
         TabIndex        =   7
         Top             =   360
         Width           =   1215
      End
      Begin VB.CheckBox RecoOther 
         Caption         =   "Reco Other Context"
         Height          =   375
         Left            =   1560
         TabIndex        =   18
         Top             =   960
         Width           =   1215
      End
      Begin VB.CheckBox PropertyNumChange 
         Caption         =   "Property Num Change"
         Height          =   375
         Left            =   120
         TabIndex        =   15
         Top             =   2400
         Width           =   1335
      End
      Begin VB.CheckBox Hypothesis 
         Caption         =   "Hypothesis"
         Height          =   255
         Left            =   120
         TabIndex        =   11
         Top             =   1440
         Width           =   1335
      End
      Begin VB.CheckBox PrivateEng 
         Caption         =   "Private"
         Height          =   255
         Left            =   120
         TabIndex        =   14
         Top             =   2160
         Width           =   1215
      End
      Begin VB.CheckBox AudioLevel 
         Caption         =   "Audio Level"
         Height          =   255
         Left            =   120
         TabIndex        =   8
         Top             =   600
         Width           =   1695
      End
   End
   Begin MSComDlg.CommonDialog ComDlg 
      Left            =   8880
      Top             =   5280
      _ExtentX        =   847
      _ExtentY        =   847
      _Version        =   393216
   End
   Begin VB.CheckBox ShowStreamInfo 
      Caption         =   "Show Stream Info"
      Height          =   255
      Left            =   7800
      TabIndex        =   25
      Top             =   480
      Value           =   1  'Checked
      Width           =   1815
   End
   Begin VB.Frame Frame2 
      Caption         =   "Recognition Type"
      Height          =   975
      Left            =   6240
      TabIndex        =   30
      Top             =   840
      Width           =   3135
      Begin VB.OptionButton Dict 
         Caption         =   "Dictation"
         Height          =   255
         Left            =   120
         TabIndex        =   29
         Top             =   600
         Width           =   1455
      End
      Begin VB.OptionButton CandC 
         Caption         =   "C&&C"
         Height          =   255
         Left            =   120
         TabIndex        =   2
         Top             =   240
         Value           =   -1  'True
         Width           =   1455
      End
   End
   Begin VB.CommandButton Recognition 
      Caption         =   "Start Recognition"
      Height          =   375
      Left            =   6240
      TabIndex        =   1
      Top             =   0
      Width           =   3135
   End
   Begin VB.Frame Frame1 
      Caption         =   "Emulate Recognition"
      Height          =   1575
      Left            =   6240
      TabIndex        =   31
      Top             =   3600
      Width           =   3135
      Begin VB.TextBox EmulateRecoTxt 
         Height          =   855
         Left            =   120
         MultiLine       =   -1  'True
         TabIndex        =   5
         Top             =   240
         Width           =   2895
      End
      Begin VB.CommandButton Emulate 
         Caption         =   "Emulate"
         Height          =   255
         Left            =   120
         TabIndex        =   6
         Top             =   1200
         Width           =   2895
      End
   End
   Begin VB.TextBox EventTextField 
      Height          =   2175
      Left            =   120
      Locked          =   -1  'True
      MultiLine       =   -1  'True
      ScrollBars      =   3  'Both
      TabIndex        =   0
      Top             =   6480
      Width           =   6015
   End
   Begin VB.Label CCLabel 
      Caption         =   "Current C&C Grammar:"
      Height          =   375
      Left            =   6240
      TabIndex        =   32
      Top             =   5400
      Width           =   3135
   End
   Begin VB.Menu File 
      Caption         =   "&File"
      Begin VB.Menu Exit 
         Caption         =   "E&xit"
      End
   End
   Begin VB.Menu MenuRecognition 
      Caption         =   "&Recognition"
      Begin VB.Menu LoadGrammar 
         Caption         =   "&Load Grammar..."
      End
   End
   Begin VB.Menu Help 
      Caption         =   "&Help"
      Begin VB.Menu About 
         Caption         =   "&About"
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
' This RecoVB App sample demonstrates most of the SR functionalities
' supported in SAPI 5.1. The main object used here for the RecoContext is RC.
'
' Copyright @ 2001 Microsoft Corporation All Rights Reserved.
'
'=============================================================================

Option Explicit

Public WithEvents RC As SpSharedRecoContext 'The main shared Recognizer Context
Attribute RC.VB_VarHelpID = -1
Public Grammar As ISpeechRecoGrammar        'Command and Control interface
Dim indent As Integer                       'Sets indent level for small output window
Dim fRecoEnabled As Boolean                 'Is recognition enabled
Dim fGrammarLoaded As Boolean               'Is a grammar loaded
Dim RecoResult As ISpeechRecoResult         'Recognition result interface

Private Sub Form_Load()
'   Set up error handler
    On Error GoTo Err_SAPILoad
    
'   Initialize globals
    indent = 0
    fRecoEnabled = False
    fGrammarLoaded = False
    
'   Create the Shared Reco Context by default
    Set RC = New SpSharedRecoContext
    
'   Load the SR Engines combo box
    Dim Token As ISpeechObjectToken
    For Each Token In RC.Recognizer.GetRecognizers
        SREngines.AddItem Token.GetDescription()
    Next
    SREngines.ListIndex = 0
    
'   Disable combo box for Shared Engine. Also disable other UI that's not initially needed.
    SREngines.Enabled = False
    ActivateMic.Enabled = False
    PlayAudio.Enabled = False
        
'   Call the InitEventInterestCheckBoxes subroutine which uses the SR engine
'   default event interests to initialize the event interest checkboxes.
    InitEventInterestCheckBoxes

'   Create grammar objects
    LoadGrammarObj
   
'   Attempt to load the default .xml file and set the RuleId State to Inactive until
'   the user starts recognition.
    LoadDefaultCnCGrammar
        
    Exit Sub
    
Err_SAPILoad:
    MsgBox "Error loading SAPI objects! Please make sure SAPI5.1 is correctly installed.", vbCritical
    Exit_Click
    Exit Sub
End Sub

'   This subroutine creates the Grammar object and sets the states to inactive
'   until the user is ready to begin recognition.
Private Sub LoadGrammarObj()
    Set Grammar = RC.CreateGrammar(1)
    
'   Load Dictation but set it to Inactive until user selects "Dictation" radio
'   button.
    Grammar.DictationLoad "", SLOStatic
    Grammar.DictationSetState SGDSInactive
End Sub

'   This subroutine attempts to load the default English .xml file. It will prompt the
'   user to load a valid .xml file if it cannot find sol.xml in either of the 2
'   specified paths.
Private Sub LoadDefaultCnCGrammar()
'   First load attempt
    On Error Resume Next
    Grammar.CmdLoadFromFile "..\\samples\\common\\sol.xml", SLODynamic
    
'   Second load attempt
    If Err Then
    On Error GoTo Err_CFGLoad
    Grammar.CmdLoadFromFile "..\\..\\common\\sol.xml", SLODynamic
    End If
    
'   Set rule state to inactive until user clicks Recognition button
    Grammar.CmdSetRuleIdState 0, SGDSInactive
    
'   Set the Label to indictate which .xml file is loaded.
    CCLabel.Caption = "Current C+C Grammar: sol.xml"
    
    fGrammarLoaded = True
    
    Exit Sub
    
Err_CFGLoad:
    fGrammarLoaded = False
    CCLabel.Caption = "Current C+C Grammar: NULL"
    Exit Sub
End Sub

'   This subroutine calls the Common File Dialog control which is inserted into
'   the form to select a .xml grammar file.
Private Sub LoadGrammar_Click()
    ComDlg.CancelError = True
    On Error GoTo Cancel
    ComDlg.Flags = cdlOFNOverwritePrompt + cdlOFNPathMustExist + cdlOFNNoReadOnlyReturn
    ComDlg.DialogTitle = "Open XML File"
    ComDlg.Filter = "All Files (*.*)|*.*|XML Files " & "(*.xml)|*.xml"
    ComDlg.FilterIndex = 2
    ComDlg.ShowOpen
        
'   Inactivate the grammar and associate a new .xml file with the grammar.
    On Error GoTo Err_XMLLoad
    Grammar.CmdLoadFromFile ComDlg.FileName, SLODynamic
    Grammar.CmdSetRuleIdState 0, SGDSInactive
        
'   Then reactivate the grammar if it was currently enabled
    If fRecoEnabled Then
        Grammar.CmdSetRuleIdState 0, SGDSActive
    End If
    
'   Set the Label to indictate which .xml file is loaded
    CCLabel.Caption = "Current C+C Grammar: " + ComDlg.FileTitle
    
    fGrammarLoaded = True
    Exit Sub
    
Err_XMLLoad:
    fGrammarLoaded = False
    MsgBox "Invalid .xml file. Please load a valid .xml grammar file.", vbOKOnly
    Exit Sub
    
Cancel:
    Exit Sub
End Sub

'   Activates or Deactivates either Command and Control or Dictation based on the
'   current state of the Recognition button.
Private Sub Recognition_Click()
    ActivateMic.Value = Checked
    
    On Error GoTo ErrorHandle

'   First make sure a valid .xml file is loaded if the user is selecting C&C
    If fGrammarLoaded Or Dict.Value = True Then
    
    '   If recognition is currently not enabled, enable it.
        If Not fRecoEnabled Then
    '       Determine if user wants to activate Dictation or Command and Control
            If Dict.Value Then
                Grammar.DictationSetState SGDSActive
            Else
                Grammar.CmdSetRuleIdState 0, SGDSActive
            End If
            fRecoEnabled = True
    '       Update caption on button.
            Recognition.Caption = "Stop Recognition"
    '       Allow user to activate/deactivate mute
            ActivateMic.Enabled = True
    '       Disable radio buttons and engines combo while recognizing so user doesn't
    '       switch modes during recognition.
            CandC.Enabled = False
            Dict.Enabled = False
            SREngines.Enabled = False
            SharedRC.Enabled = False
            Inproc.Enabled = False
    '   If recognition is currently enabled, disable it.
        Else
    '       Deactivate either Dictation or Command and Control.
            If Dict.Value Then
                Grammar.DictationSetState SGDSInactive
            Else
                Grammar.CmdSetRuleIdState 0, SGDSInactive
            End If
            fRecoEnabled = False
    '       Update caption on button.
            Recognition.Caption = "Start Recognition"
    '       Disallow user to activate/deactivate mute
            ActivateMic.Enabled = False
    '       Reenable radio buttons while not recognizing so user can switch modes
            CandC.Enabled = True
            Dict.Enabled = True
            
    '       Allow engine selection if the inproc button is selected.
            If Inproc.Value Then
                       SREngines.Enabled = True
            End If
            
            SharedRC.Enabled = True
            Inproc.Enabled = True
        End If
    End If
    Exit Sub
    
ErrorHandle:
    MsgBox "Failed to activate the grammar. It is possible that your audio device is used by other application.", vbOKOnly
End Sub

'   This subroutine calls the EmulateRecognition method on the recognizer.
Private Sub Emulate_Click()
    If fGrammarLoaded Then
'       First make sure not to try retaining audio when emulating.
        RetainAudio.Value = Unchecked
        
'       Temporarily enable recognition state if it's not currently enabled.
        If Not fRecoEnabled Then
            If Dict.Value Then
                Grammar.DictationSetState SGDSActive
            Else
                Grammar.CmdSetRuleIdState 0, SGDSActive
            End If
        End If

'       Call the EmmulateRecognition method with the text from the textbox.
        If Not EmulateRecoTxt.Text = "" Then
            RC.Recognizer.EmulateRecognition EmulateRecoTxt.Text, 0
        End If

'       Reset the recognition states to original values
        If Not fRecoEnabled Then
            If Dict.Value Then
                Grammar.DictationSetState SGDSInactive
            Else
                Grammar.CmdSetRuleIdState 0, SGDSInactive
            End If
        End If
    End If
End Sub

'   This subroutine is called by the event handler subroutines to update
'   the small edit window with the events that were received.
Private Sub UpdateEventList(StreamNum As Long, StreamPos As Variant, szEvent As String, szEventInfo As String)
    Dim szIndent As String
    Dim szStreamInfo As String
    Dim i As Integer
    
    For i = 0 To indent - 1
        szIndent = szIndent & "    "
    Next
    If ShowStreamInfo.Value = Checked Then
        szStreamInfo = " (StreamNum=" & StreamNum & " StreamPos=" & StreamPos & ")"
    End If
    EventTextField.Text = EventTextField.Text & szIndent & szEvent & szStreamInfo & szEventInfo & vbCrLf
    EventTextField.SelStart = Len(EventTextField)
    EventTextField.SelLength = 0
End Sub

'   The following subroutines are event handlers that get called when the SR engine
'   fires events.

'   Adaption event handler
Private Sub RC_Adaption(ByVal StreamNumber As Long, ByVal StreamPosition As Variant)
    UpdateEventList StreamNumber, StreamPosition, "Adaptation", ""
End Sub
'   Audio Level event handler
Private Sub RC_AudioLevel(ByVal StreamNumber As Long, ByVal StreamPosition As Variant, ByVal AudioLevel As Long)
    UpdateEventList StreamNumber, StreamPosition, "AudioLevel", " [Level=" & AudioLevel & "]"
End Sub
'   Bookmark event handler
Private Sub RC_Bookmark(ByVal StreamNumber As Long, ByVal StreamPosition As Variant, ByVal EventData As Variant, ByVal Options As SpeechLib.SpeechBookmarkOptions)
    UpdateEventList StreamNumber, StreamPosition, "Bookmark", " [Data=" & EventData & " Option=" & Options & "]"
End Sub
'   End Stream event handler
Private Sub RC_EndStream(ByVal StreamNumber As Long, ByVal StreamPosition As Variant, ByVal StreamReleased As Boolean)
    indent = 0
    UpdateEventList StreamNumber, StreamPosition, "EndStream", " [Stream Released=" & StreamReleased & "]"
End Sub
'   Engine Private event handler
Private Sub RC_EnginePrivate(ByVal StreamNumber As Long, ByVal StreamPosition As Variant, ByVal lParam As Variant)
    UpdateEventList StreamNumber, StreamPosition, "EnginePrivate", ""
End Sub
'   False Recognition event handler
Private Sub RC_FalseRecognition(ByVal StreamNumber As Long, ByVal StreamPosition As Variant, ByVal Result As SpeechLib.ISpeechRecoResult)
    UpdateEventList StreamNumber, StreamPosition, "FalseRecognition", " [Text=" & Result.PhraseInfo.GetText() & "]"
    Set RecoResult = Result
End Sub
'   Hypothesis event handler
Private Sub RC_Hypothesis(ByVal StreamNumber As Long, ByVal StreamPosition As Variant, ByVal Result As SpeechLib.ISpeechRecoResult)
    UpdateEventList StreamNumber, StreamPosition, "Hypothesis", " [Text=" & Result.PhraseInfo.GetText() & "]"
End Sub
'   Interference event handler
Private Sub RC_Interference(ByVal StreamNumber As Long, ByVal StreamPosition As Variant, ByVal Interference As SpeechLib.SpeechInterference)
    UpdateEventList StreamNumber, StreamPosition, "Interference", " [Value=" & Interference & "]"
End Sub
'   Phrase Start event handler
Private Sub RC_PhraseStart(ByVal StreamNumber As Long, ByVal StreamPosition As Variant)
    UpdateEventList StreamNumber, StreamPosition, "PhraseStart", ""
End Sub
'   Property Number Change event handler
Private Sub RC_PropertyNumberChange(ByVal StreamNumber As Long, ByVal StreamPosition As Variant, ByVal PropertyName As String, ByVal NewNumberValue As Long)
    UpdateEventList StreamNumber, StreamPosition, "PropertyNumberChange", " [Name=" & PropertyName & " Value=" & NewNumberValue & "]"
End Sub
'   Property String Change event handler
Private Sub RC_PropertyStringChange(ByVal StreamNumber As Long, ByVal StreamPosition As Variant, ByVal PropertyName As String, ByVal NewStringValue As String)
    UpdateEventList StreamNumber, StreamPosition, "PropertyStringChange", " [Name=" & PropertyName & " Value=" & NewStringValue & "]"
End Sub
'   Recognition event handler
Private Sub RC_Recognition(ByVal StreamNumber As Long, ByVal StreamPosition As Variant, ByVal RecognitionType As SpeechLib.SpeechRecognitionType, ByVal Result As SpeechLib.ISpeechRecoResult)
    Dim RecoNode As Node
    Static i As Integer
    
'   Update Event List window first
    UpdateEventList StreamNumber, StreamPosition, "Recognition", " [Text=" & Result.PhraseInfo.GetText() & ", RecoType=" & RecognitionType & "]"
     
'   Increment unique value for RecoNode's key name.
    i = i + 1
     
'   Add top level node
    Set RecoNode = TreeView1.Nodes.Add(, , "Reco" & i, "Recognition (" & Result.PhraseInfo.GetText() & ")")

'   Call the BuildResultTree subroutine to build up the Result tree
    BuildResultTree Result.PhraseInfo, Result.Alternates(5), RecoNode
    
'   Save the recognition Result to the global RecoResult
    Set RecoResult = Result
End Sub
'   Recognition For Other Context event handler
Private Sub RC_RecognitionForOtherContext(ByVal StreamNumber As Long, ByVal StreamPosition As Variant)
    UpdateEventList StreamNumber, StreamPosition, "RecognitionForOtherContext", ""
End Sub
'   Recognizer State Change event handler
Private Sub RC_RecognizerStateChange(ByVal StreamNumber As Long, ByVal StreamPosition As Variant, ByVal NewState As SpeechLib.SpeechRecognizerState)
    UpdateEventList StreamNumber, StreamPosition, "RecognitionStateChange", " [NewState=" & NewState & "]"
End Sub
'   Request UI event handler
Private Sub RC_RequestUI(ByVal StreamNumber As Long, ByVal StreamPosition As Variant, ByVal UIType As String)
    UpdateEventList StreamNumber, StreamPosition, "RequestUI", " [Type=" & UIType & "]"
End Sub
'   Sound End event handler
Private Sub RC_SoundEnd(ByVal StreamNumber As Long, ByVal StreamPosition As Variant)
    indent = 1
    UpdateEventList StreamNumber, StreamPosition, "SoundEnd", ""
End Sub
'   Sound Start event handler
Private Sub RC_SoundStart(ByVal StreamNumber As Long, ByVal StreamPosition As Variant)
    indent = 1
    UpdateEventList StreamNumber, StreamPosition, "SoundStart", ""
    indent = 2
End Sub
'   Stream Start event handler
Private Sub RC_StartStream(ByVal StreamNumber As Long, ByVal StreamPosition As Variant)
    indent = 0
    UpdateEventList StreamNumber, StreamPosition, "StartStream", ""
    indent = 1
End Sub

'   The following subroutine enables the event interest check boxes based on the
'   SR engine's default event interests.
Private Sub InitEventInterestCheckBoxes()
    If RC.EventInterests And SREAdaptation Then
        Adaption.Value = Checked
    End If
    
    If RC.EventInterests And SREAudioLevel Then
        AudioLevel.Value = Checked
    End If
    
    If RC.EventInterests And SREBookmark Then
        Bookmark.Value = Checked
    End If
    
    If RC.EventInterests And SREFalseRecognition Then
        FalseReco.Value = Checked
    End If
    
    If RC.EventInterests And SREHypothesis Then
        Hypothesis.Value = Checked
    End If
    
    If RC.EventInterests And SREInterference Then
        Interference.Value = Checked
    End If
    
    If RC.EventInterests And SREPhraseStart Then
        PhraseStart.Value = Checked
    End If
    
    If RC.EventInterests And SREPrivate Then
        PrivateEng.Value = Checked
    End If
    
    If RC.EventInterests And SREPropertyNumChange Then
        PropertyNumChange.Value = Checked
    End If
    
    If RC.EventInterests And SREPropertyStringChange Then
        PropertyStringChange.Value = Checked
    End If
    
    If RC.EventInterests And SRERecognition Then
        Reco.Value = Checked
    End If
    
    If RC.EventInterests And SRERecoOtherContext Then
        RecoOther.Value = Checked
    End If
    
    If RC.EventInterests And SRERequestUI Then
        RequestUI.Value = Checked
    End If
    
    If RC.EventInterests And SRESoundEnd Then
        SoundEnd.Value = Checked
    End If
    
    If RC.EventInterests And SRESoundStart Then
        SoundStart.Value = Checked
    End If
        
    If RC.EventInterests And SREStateChange Then
        StateChange.Value = Checked
    End If
    
    If RC.EventInterests And SREStreamEnd Then
        StreamEnd.Value = Checked
    End If
    
    If RC.EventInterests And SREStreamStart Then
        StreamStart.Value = Checked
    End If
End Sub

'   The following subroutines handle turning on/off the event interests when
'   The user checks/unchecks them.

Private Sub SetEventInterest(EventInterest As SpeechRecoEvents, EventCheckBox As CheckBox)
    If EventCheckBox.Value = Checked Then
        RC.EventInterests = RC.EventInterests Or EventInterest
    Else
        RC.EventInterests = RC.EventInterests And Not EventInterest
    End If
End Sub

'   Adaption event interest
Private Sub Adaption_Click()
    SetEventInterest SREAdaptation, Adaption
End Sub
'   Audio Level event interest
Private Sub AudioLevel_Click()
    SetEventInterest SREAudioLevel, AudioLevel
End Sub
'   Bookmark event interest
Private Sub Bookmark_Click()
    SetEventInterest SREBookmark, Bookmark
End Sub
'   False Recognition event interest
Private Sub FalseReco_Click()
    SetEventInterest SREFalseRecognition, FalseReco
End Sub
'   Hypothesis event interest
Private Sub Hypothesis_Click()
    SetEventInterest SREHypothesis, Hypothesis
End Sub
'   Interference event interest
Private Sub Interference_Click()
    SetEventInterest SREInterference, Interference
End Sub
'   Phrase Start event interest
Private Sub PhraseStart_Click()
    SetEventInterest SREPhraseStart, PhraseStart
End Sub
'   Engine Private event interest
Private Sub PrivateEng_Click()
    SetEventInterest SREPrivate, PrivateEng
End Sub
'   Property Number Change event interest
Private Sub PropertyNumChange_Click()
    SetEventInterest SREPropertyNumChange, PropertyNumChange
End Sub
'   Property String Change event interest
Private Sub PropertyStringChange_Click()
    SetEventInterest SREPropertyStringChange, PropertyStringChange
End Sub
'   Recognition event interest
Private Sub Reco_Click()
    SetEventInterest SRERecognition, Reco
End Sub
'   Recognition on Other Context event interest
Private Sub RecoOther_Click()
    SetEventInterest SRERecoOtherContext, RecoOther
End Sub
'   Request UI event interest
Private Sub RequestUI_Click()
    SetEventInterest SRERequestUI, RequestUI
End Sub
'   Sound End event interest
Private Sub SoundEnd_Click()
    SetEventInterest SRESoundEnd, SoundEnd
End Sub
'   Sound Start event interest
Private Sub SoundStart_Click()
    SetEventInterest SRESoundStart, SoundStart
End Sub
'   State Change event interest
Private Sub StateChange_Click()
    SetEventInterest SREStateChange, StateChange
End Sub
'   Stream Start event interest
Private Sub StreamStart_Click()
    SetEventInterest SREStreamStart, StreamStart
End Sub
'   Stream End event interest
Private Sub StreamEnd_Click()
    SetEventInterest SREStreamEnd, StreamEnd
End Sub

'   The following subroutines use the Recognition Result object to build up the
'   TreeView display in the main window.

'   This subroutine builds up the result tree in the main treeview window. It uses
'   the main ISpeechRecoResult object to build up this information. Additionally it
'   also shows the alternates from the recognition.
Private Sub BuildResultTree(ByVal ResultPhraseInfo As ISpeechPhraseInfo, ByVal Alternates As ISpeechPhraseAlternates, ParentNode As Node, Optional DontDoAlternates As Boolean = False)
    Dim id As Integer
    id = TreeView1.Nodes.Count
    Dim PhraseInfoNode As Node
    Dim PropertiesNode As Node
    
'   Add the top level nodes for the result tree
    Set PhraseInfoNode = TreeView1.Nodes.Add(ParentNode.Key, tvwChild, "PhraseInfo" & id, "Phrase Info")
    Set PropertiesNode = TreeView1.Nodes.Add(PhraseInfoNode.Key, tvwChild, "Properties" & id, "Properties")
    
'   Call subroutines to build up lower level nodes for the Result rules, Result properties,
'   and Result elements.
    BuildPhraseRuleTree ResultPhraseInfo.Rule, PhraseInfoNode
    BuildPhrasePropertyTree ResultPhraseInfo.Properties, PropertiesNode
    BuildPhraseElementsTree ResultPhraseInfo.Elements, PhraseInfoNode, ResultPhraseInfo.LanguageId
    
'   Call subroutine to build up lower level nodes for the alternates.
    If Not DontDoAlternates And Not Alternates Is Nothing Then
        Dim AltNode As Node
        Set AltNode = TreeView1.Nodes.Add(ParentNode.Key, tvwChild, "Alternates" & id, "Alternates")
        BuildAlternatesTree Alternates, AltNode
    End If
End Sub
'   This subroutine is called by the BuildResultTree subroutine to build up the Result
'   rules.
Private Sub BuildPhraseRuleTree(ByVal PhraseRule As ISpeechPhraseRule, ParentNode As Node, Optional i As Integer = 0)
    Dim Rule As ISpeechPhraseRule
    Dim RuleNode As Node
    
    Set RuleNode = TreeView1.Nodes.Add(ParentNode.Key, tvwChild, "RuleNode" & ParentNode.Key & i, "Rule (" & PhraseRule.Name & ")")
                
'   Call the subroutine recusively if you have child rules
    If Not PhraseRule.Children Is Nothing Then
        For Each Rule In PhraseRule.Children
            i = i + 1
            BuildPhraseRuleTree Rule, RuleNode, i
        Next
    End If
End Sub
'   This subroutine is called by the BuildResultTree subroutine to build up the Result
'   properties.
Private Sub BuildPhrasePropertyTree(ByVal Properties As ISpeechPhraseProperties, ParentNode As Node)
    Dim Property As ISpeechPhraseProperty
    Dim i As Integer
    i = 0
    
    If Not Properties Is Nothing Then
        If Properties.Count > 0 Then
            For Each Property In Properties
                Dim PropertyNode As Node
                Dim DisplayString As String
                i = i + 1
                                
                If Property.Value = Empty Then
                    DisplayString = "Property (" & Property.Name & ")"
                Else
                    DisplayString = "Property (" & Property.Name & ") (" & Property.Value & ")"
                End If
                Set PropertyNode = TreeView1.Nodes.Add(ParentNode.Key, tvwChild, ParentNode.Key & i, DisplayString)
                
'               Call the subroutine recusively if you have child properties
                If Not Property.Children Is Nothing Then
                    BuildPhrasePropertyTree Property.Children, PropertyNode
                End If
            Next
        End If
    Else
        ParentNode.Text = "Properties - No Properties"
    End If
End Sub
'   This subroutine is called by the BuildResultTree subroutine to build up the Result
'   elements.
Private Sub BuildPhraseElementsTree(ByVal Elements As ISpeechPhraseElements, ParentNode As Node, LangId As Long)
    Dim Element As ISpeechPhraseElement
    Dim ElementsNode As Node
    Dim i As Integer
    i = 0
    
    If Not Elements Is Nothing Then
        If Elements.Count > 0 Then
            Set ElementsNode = TreeView1.Nodes.Add(ParentNode.Key, tvwChild, "Elements" & ParentNode.Key, "Elements")
            For Each Element In Elements
                i = i + 1
                Dim ElementNode As Node
                Set ElementNode = TreeView1.Nodes.Add(ElementsNode.Key, tvwChild, "Element" & ParentNode.Key & i, "Element (" & Element.DisplayText & ")")
                If Not IsEmpty(Element.Pronunciation) Then
                    BuildPronunciationTree Element.Pronunciation, ElementNode, LangId
                End If
            Next
        End If
    Else
        Set ElementNode = TreeView1.Nodes.Add(ParentNode.Key, tvwChild, "Elements" & ParentNode.Key, "Elements - No Elements")
    End If
End Sub
'   This subroutine is called by the BuildPhraseElementsTree subroutine to build up the
'   Result pronunciations for each of the Result elements.
Private Sub BuildPronunciationTree(ByVal Pronunciation As Variant, ParentNode As Node, LangId As Long)
    Dim PronunciationNode As Node
    Dim PC As New SpPhoneConverter
    Dim i As Integer
    i = 0
    
'   Initialize the Phone Converter
    PC.LanguageId = LangId
    
    Set PronunciationNode = TreeView1.Nodes.Add(ParentNode.Key, tvwChild, "Pronunciations" & ParentNode.Key, "Pronunciations")
    
    For i = 0 To UBound(Pronunciation)
        Dim Phone As String
        Phone = PC.IdToPhone(Pronunciation(i))
        Call TreeView1.Nodes.Add(PronunciationNode.Key, tvwChild, "Pronunciation" & ParentNode.Key & i, "Pronunciation (" & Phone & ")")
    Next
End Sub

'   This subroutine is called by the BuildResultTree subroutine to build up the
'   Result alternates if there are any.
Private Sub BuildAlternatesTree(ByVal Alternates As ISpeechPhraseAlternates, ParentNode As Node)
    Dim alt As ISpeechPhraseAlternate
    Dim i As Integer
    i = 0
    
    For Each alt In Alternates
        i = i + 1
        Dim AltNode As Node
        Set AltNode = TreeView1.Nodes.Add(ParentNode.Key, tvwChild, ParentNode.Key & i, "Alternate" & i)
        BuildResultTree alt.PhraseInfo, Nothing, AltNode, True
    Next
End Sub

'   The following 2 subroutines destroy/create Inproc and Shared RecoContext's depending
'   on what the user has checked.

'   This subroutine destroys the Inproc RecoContext and creates and Shared RecoContext
Private Sub SharedRC_Click()
'   Destroy the Inproc RecoContext
    Set RC = Nothing
    
'   Create the Shared RecoContext
    Set RC = New SpSharedRecoContext
    
'   Call the InitEventInterestCheckBoxes subroutine which uses the SR engine
'   default event interests to initialize the event interest checkboxes.
    InitEventInterestCheckBoxes

'   Create grammar objects
    LoadGrammarObj
   
'   Attempt to load the default .xml file and set the RuleId State to Inactive until
'   the user starts recognition.
    LoadDefaultCnCGrammar
    
'   Disable the engine selection drop down box and reset to the default shared engine.
    SREngines.ListIndex = 0
    SREngines.Enabled = False
End Sub
'   This subroutine destroys the Shared RecoContext and creates and Inproc RecoContext
Private Sub Inproc_Click()
    Dim Recognizer As ISpeechRecognizer
    
'   Destroy Shared RecoContext
    Set RC = Nothing
    
'   Create Inproc Recognizer which we will use to create the Inproc RecoContext.
    Set Recognizer = New SpInprocRecognizer

'   To create an Inproc RecoContext we must set an Audio Input. To do this we create
'   an SpObjectTokenCategory object with the category of AudioIn. This object enumerates
'   the registry to see what types of audio input devices are available.
    Dim ObjectTokenCat As ISpeechObjectTokenCategory
    Set ObjectTokenCat = New SpObjectTokenCategory
    ObjectTokenCat.SetId SpeechCategoryAudioIn

'   Set the default AudioInput device which is typically the first item and is usually
'   the microphone.
    Set Recognizer.AudioInput = ObjectTokenCat.EnumerateTokens.Item(0)
    
'   Set the Recognizer to the one selected in the drop down box.
    Set Recognizer.Recognizer = Recognizer.GetRecognizers().Item(SREngines.ListIndex)

'   Now go ahead and actually create the Inproc RecoContext.
'   Note - in VB even though the global "RC" object is declaired as a
'   SpSharedRecoContext, it is still possible to set it to a SpInprocRecoContext.
    Set RC = Recognizer.CreateRecoContext
    
'   Call the InitEventInterestCheckBoxes subroutine which uses the SR engine
'   default event interests to initialize the event interest checkboxes.
    InitEventInterestCheckBoxes

'   Create grammar objects
    LoadGrammarObj
   
'   Attempt to load the default .xml file and set the RuleId State to Inactive until
'   the user starts recognition.
    LoadDefaultCnCGrammar
    
'   Enable the engine selection drop down box
    SREngines.Enabled = True
End Sub

'   The remaining subroutines handle simple UI and exiting.

'   This subroutine activates/deactivates the microphone.
Private Sub ActivateMic_Click()
    If ActivateMic.Value = Checked Then
        If Dict.Value Then
            Grammar.DictationSetState SGDSActive
        Else
            Grammar.CmdSetRuleIdState 0, SGDSActive
        End If
    Else
        If Dict.Value Then
            Grammar.DictationSetState SGDSInactive
        Else
            Grammar.CmdSetRuleIdState 0, SGDSInactive
        End If
    End If
End Sub
'   This subroutine clears the event text box
Private Sub ClearEvents_Click()
    EventTextField.Text = ""
End Sub
'   This subroutine clears the TreeView window
Private Sub ClearTree_Click()
    TreeView1.Nodes.Clear
End Sub
'   This subroutine changes the SR Engine to the selected one
Private Sub SREngines_Click()
'   This subroutine can be called when you update the listindex of SREngines in the form load subroutine
    If Inproc.Value Then
        Inproc_Click
    End If
End Sub
'   Set the 'Emulate' button to be the default when the user is typing text into
'   the EmulateRecoTxt text box.
Private Sub EmulateRecoTxt_Change()
    Emulate.Default = True
End Sub
'   Retain the audio
Private Sub RetainAudio_Click()
    If RetainAudio.Value = Checked Then
        RC.RetainedAudio = SRAORetainAudio
        PlayAudio.Enabled = True
    Else
        RC.RetainedAudio = SRAONone
        PlayAudio.Enabled = False
    End If

'   Clear out any old recognition results
    Set RecoResult = Nothing
End Sub
'   Play the Retained Audio
Private Sub PlayAudio_Click()
    If Not RecoResult Is Nothing Then
        RecoResult.SpeakAudio
    End If
End Sub
'   About box
Private Sub About_Click()
    MsgBox "(c) 2001 Microsoft Corporation. All rights reserved.", vbInformation, "About RecoVB"
End Sub
Private Sub ExitBtn_Click()
    Unload Form1
End Sub
Private Sub Exit_Click()
    Unload Form1
End Sub
