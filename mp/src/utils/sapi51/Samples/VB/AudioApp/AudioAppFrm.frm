VERSION 5.00
Object = "{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}#2.0#0"; "mscomctl.ocx"
Begin VB.Form AudioAppFrm 
   BorderStyle     =   3  'Fixed Dialog
   Caption         =   "AudioApp"
   ClientHeight    =   5310
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   3705
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   5310
   ScaleWidth      =   3705
   ShowInTaskbar   =   0   'False
   StartUpPosition =   3  'Windows Default
   Begin MSComctlLib.StatusBar AudioStatusBar 
      Align           =   2  'Align Bottom
      Height          =   255
      Left            =   0
      TabIndex        =   4
      Top             =   5055
      Width           =   3705
      _ExtentX        =   6535
      _ExtentY        =   450
      Style           =   1
      _Version        =   393216
      BeginProperty Panels {8E3867A5-8586-11D1-B16A-00C0F0283628} 
         NumPanels       =   1
         BeginProperty Panel1 {8E3867AB-8586-11D1-B16A-00C0F0283628} 
         EndProperty
      EndProperty
   End
   Begin VB.TextBox SpeakTxt 
      Height          =   1095
      Left            =   240
      MultiLine       =   -1  'True
      ScrollBars      =   2  'Vertical
      TabIndex        =   0
      Text            =   "AudioAppFrm.frx":0000
      Top             =   720
      Width           =   3015
   End
   Begin VB.CommandButton SpeakBtn 
      Caption         =   "Reco From TTS"
      Default         =   -1  'True
      Height          =   375
      Left            =   240
      TabIndex        =   2
      Top             =   4080
      Width           =   1335
   End
   Begin VB.CommandButton ExitBtn 
      Caption         =   "Exit"
      Height          =   375
      Left            =   2040
      TabIndex        =   1
      Top             =   4080
      Width           =   1215
   End
   Begin VB.TextBox Recotxt 
      Height          =   1095
      Left            =   240
      Locked          =   -1  'True
      MultiLine       =   -1  'True
      ScrollBars      =   2  'Vertical
      TabIndex        =   3
      Top             =   2640
      Width           =   3015
   End
   Begin VB.Label Label2 
      Caption         =   "Recognition Results:"
      Height          =   255
      Left            =   240
      TabIndex        =   6
      Top             =   2280
      Width           =   1935
   End
   Begin VB.Label Label1 
      Caption         =   "Enter text to recognize:"
      Height          =   255
      Left            =   240
      TabIndex        =   5
      Top             =   360
      Width           =   1815
   End
End
Attribute VB_Name = "AudioAppFrm"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
'=============================================================================
'
' AudioAppFrm
'
' Copyright @ 2001 Microsoft Corporation All Rights Reserved.
'=============================================================================
Option Explicit

Const AUDIOFORMAT = SAFT8kHz16BitMono

'tts variables
Dim WithEvents Voice As SpVoice
Attribute Voice.VB_VarHelpID = -1
Dim EndofStream As Boolean
Dim AudioPlugOut As SpAudioPlug

'sr variables
Dim WithEvents RecoContext As SpInProcRecoContext
Attribute RecoContext.VB_VarHelpID = -1
Dim Grammar As ISpeechRecoGrammar
Dim Recognizer As SpInprocRecognizer
Dim AudioPlugIn As SpAudioPlug


Private Sub ExitBtn_Click()
    Grammar.DictationSetState SGDSInactive
    Unload AudioAppFrm
End Sub





Private Sub Form_Load()

    Set Voice = New SpVoice
    EndofStream = False
    
    'Set up the output audio object
    Set AudioPlugOut = New SpAudioPlug
    AudioPlugOut.Init True, AUDIOFORMAT
    Set Voice.AudioOutputStream = AudioPlugOut
    
    
    Debug.Print "Initializing SAPI reco context object..."
    Set Recognizer = New SpInprocRecognizer
    
    'Set up the input audio object
    Set AudioPlugIn = New SpAudioPlug
    AudioPlugIn.Init False, AUDIOFORMAT
    Set Recognizer.AudioInputStream = AudioPlugIn
    
    
    Set RecoContext = Recognizer.CreateRecoContext
    Set Grammar = RecoContext.CreateGrammar(1)
    Grammar.DictationLoad
    
   

    
 
End Sub

Public Sub PlayPlug()

    On Error GoTo Cancel
    
    Dim output As Variant

    Recotxt.Text = ""
    Voice.Speak SpeakTxt.Text, SVSFlagsAsync

    EndofStream = False
    
    'Update the status bar, before we start the feed the audio
    AudioStatusBar.SimpleText = "Feeding TTS audio to SR..."
    AudioStatusBar.Refresh
    
    Grammar.DictationSetState SGDSActive
   
    Do While (EndofStream = False)
    
        'We need to process the message in the message queue
        DoEvents
        
        'Get the audio data from the audio object
        output = AudioPlugOut.GetData

        
        
        'Output the audio data to the input audio object
        If (Len(output) * 2 <> 0) Then
            AudioPlugIn.SetData (output)
        End If
        Sleep (500)
    Loop

    'Update the status bar after the we have feed all the audio data
    AudioStatusBar.SimpleText = "SR Engine is doing dictation recognition..."
    AudioStatusBar.Refresh

Cancel:
    Exit Sub

End Sub



Private Sub Form_Unload(Cancel As Integer)
    Set AudioPlugIn = Nothing
End Sub

Private Sub RecoContext_EndStream(ByVal StreamNumber As Long, ByVal StreamPosition As Variant, ByVal StreamReleased As Boolean)

    'Update the status bar
    AudioStatusBar.SimpleText = "Recognition done"
    AudioStatusBar.Refresh
    
    'User can start another recognition
    SpeakBtn.Enabled = True
    SpeakTxt.SetFocus
    
    
    Grammar.DictationSetState SGDSInactive
    
End Sub

Private Sub RecoContext_Recognition(ByVal StreamNumber As Long, ByVal StreamPosition As Variant, ByVal RecognitionType As SpeechLib.SpeechRecognitionType, ByVal Result As SpeechLib.ISpeechRecoResult)
Recotxt.Text = Result.PhraseInfo.GetText & Recotxt.Text
End Sub

Private Sub SpeakBtn_Click()
    PlayPlug
End Sub


Private Sub Voice_EndStream(ByVal StreamNumber As Long, ByVal StreamPosition As Variant)
    EndofStream = True
   
End Sub


Private Sub Voice_StartStream(ByVal StreamNumber As Long, ByVal StreamPosition As Variant)
    SpeakBtn.Enabled = False
    
End Sub
