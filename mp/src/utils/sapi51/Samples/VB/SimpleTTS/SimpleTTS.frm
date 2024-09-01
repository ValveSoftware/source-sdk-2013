VERSION 5.00
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.2#0"; "COMDLG32.OCX"
Begin VB.Form Form1 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "Simple TTS"
   ClientHeight    =   1800
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   4125
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   120
   ScaleMode       =   3  'Pixel
   ScaleWidth      =   275
   StartUpPosition =   3  'Windows Default
   Begin MSComDlg.CommonDialog ComDlg 
      Left            =   1800
      Top             =   600
      _ExtentX        =   847
      _ExtentY        =   847
      _Version        =   393216
   End
   Begin VB.CommandButton SpeakItBtn 
      Caption         =   "Speak It"
      Default         =   -1  'True
      Height          =   375
      Left            =   1560
      TabIndex        =   3
      Top             =   1200
      Width           =   1095
   End
   Begin VB.CheckBox SaveToWavCheckBox 
      Caption         =   "Save to .wav"
      Height          =   255
      Left            =   120
      TabIndex        =   2
      ToolTipText     =   "The text is a file name"
      Top             =   1260
      Width           =   1335
   End
   Begin VB.CommandButton ExitBtn 
      Caption         =   "Exit"
      Height          =   375
      Left            =   2880
      TabIndex        =   1
      Top             =   1200
      Width           =   1095
   End
   Begin VB.TextBox TextField 
      Height          =   765
      Left            =   0
      MultiLine       =   -1  'True
      ScrollBars      =   2  'Vertical
      TabIndex        =   0
      Text            =   "SimpleTTS.frx":0000
      Top             =   120
      Width           =   4095
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
'=============================================================================
'
' This SimpleTTS sample application demonstrates how to create a SpVoice object
' and how to use it to speak text and save it to a .wav file.
'
' Copyright @ 2001 Microsoft Corporation All Rights Reserved.
'=============================================================================

Option Explicit

'Declare the SpVoice object.
Dim Voice As SpVoice
'Note - Applications that require handling of SAPI events should declair the
'SpVoice as follows:
'Dim WithEvents Voice As SpVoice

Private Sub Form_Load()
'   Initialize the voice object
    Set Voice = New SpVoice
End Sub
Private Sub ExitBtn_Click()
    Unload Form1
End Sub

Private Sub SpeakItBtn_Click()
    On Error GoTo Speak_Error
    
'   If the 'Save to wav' checkbox is checked handle this special case by
'   calling the SaveToWav() function.
    If SaveToWavCheckBox Then
        SaveToWav
    Else
'       Call the Speak method with the text from the text box. We use the
'       SVSFlagsAsync flag to speak asynchronously and return immediately
'       from this call.
        If Not TextField.Text = "" Then
            Voice.Speak TextField.Text, SVSFlagsAsync
        End If
    End If
    
'   Return focus to text box
    TextField.SetFocus
    Exit Sub
    
Speak_Error:
    MsgBox "Speak Error!", vbOKOnly
End Sub

Private Sub SaveToWav()
'   Create a wave stream
    Dim cpFileStream As New SpFileStream
    
'   Set audio format
    cpFileStream.Format.Type = SAFT22kHz16BitMono
    
'   Call the Common File Dialog control which is inserted into the form to
'   select a name for the .wav file.
    ComDlg.CancelError = True
    On Error GoTo Cancel
    ComDlg.Flags = cdlOFNOverwritePrompt + cdlOFNPathMustExist + cdlOFNNoReadOnlyReturn
    ComDlg.DialogTitle = "Save to a Wave File"
    ComDlg.Filter = "All Files (*.*)|*.*|Wave Files " & "(*.wav)|*.wav"
    ComDlg.FilterIndex = 2
    ComDlg.ShowSave
        
'   Create a new .wav file for writing. False indicates that we're not
'   interested in writing events into the .wav file.
'   Note - this line of code will fail if the file exists and is currently open.
    cpFileStream.Open ComDlg.FileName, SSFMCreateForWrite, False

'   Set the .wav file stream as the output for the Voice object
    Set Voice.AudioOutputStream = cpFileStream
    
'   Calling the Speak method now will send the output to the "SimpTTS.wav" file.
'   We use the SVSFDefault flag so this call does not return until the file is
'   completely written.
    Voice.Speak TextField.Text, SVSFDefault
    
'   Close the file
    cpFileStream.Close
    Set cpFileStream = Nothing
    
'   Reset the Voice object's output to 'Nothing'. This will force it to use
'   the default audio output the next time.
    Set Voice.AudioOutputStream = Nothing
    
Cancel:
    Exit Sub
End Sub

