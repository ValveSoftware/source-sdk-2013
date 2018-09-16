VERSION 5.00
Begin VB.Form SimpleDict 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "Simple Dictation"
   ClientHeight    =   3780
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   4680
   Icon            =   "SimpleDict.frx":0000
   LinkTopic       =   "SimpleDict"
   MaxButton       =   0   'False
   ScaleHeight     =   3780
   ScaleWidth      =   4680
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton btnStart 
      Caption         =   "St&art"
      Height          =   375
      Left            =   840
      TabIndex        =   1
      Top             =   3240
      Width           =   1215
   End
   Begin VB.CommandButton btnStop 
      Caption         =   "St&op"
      Height          =   375
      Left            =   2520
      TabIndex        =   2
      Top             =   3240
      Width           =   1215
   End
   Begin VB.TextBox txtSpeech 
      Height          =   2895
      Left            =   120
      Locked          =   -1  'True
      MultiLine       =   -1  'True
      ScrollBars      =   2  'Vertical
      TabIndex        =   0
      Top             =   120
      Width           =   4455
   End
End
Attribute VB_Name = "SimpleDict"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
'=============================================================================
' This sample demonstrates how to do simple dictation in VB with SAPI 5.1.
'
' It uses shared reco context object, uses the default audio input, loads in
' dictation grammar, sets up event handlers, and shows the recognized text in
' the dialog text box.
'
' Note: since the text box is using system locale, it may not correctly show
' characters in other languages. For example, if you use Chinese Speech
' Recognition engine as the default engine on your English OS, the text box
' may show garbage even though the engine recognizes Chinese.
'
' Copyright @ 2001 Microsoft Corporation All Rights Reserved.
'
'=============================================================================

Option Explicit

Dim WithEvents RecoContext As SpSharedRecoContext
Attribute RecoContext.VB_VarHelpID = -1
Dim Grammar As ISpeechRecoGrammar

Dim m_bRecoRunning As Boolean
Dim m_cChars As Integer


Private Sub Form_Load()
    SetState False
    m_cChars = 0
End Sub

Private Sub btnStart_Click()
    Debug.Assert Not m_bRecoRunning
    
    ' Initialize recognition context object and grammar object, then
    ' start dictation
    If (RecoContext Is Nothing) Then
        Debug.Print "Initializing SAPI reco context object..."
        Set RecoContext = New SpSharedRecoContext
        Set Grammar = RecoContext.CreateGrammar(1)
        Grammar.DictationLoad
    End If
    
    Grammar.DictationSetState SGDSActive
    SetState True
End Sub

Private Sub btnStop_Click()
    Debug.Assert m_bRecoRunning
    Grammar.DictationSetState SGDSInactive
    SetState False
End Sub

' This function handles Recognition event from the reco context object.
' Recognition event is fired when the speech recognition engines recognizes
' a sequences of words.
Private Sub RecoContext_Recognition(ByVal StreamNumber As Long, _
                                    ByVal StreamPosition As Variant, _
                                    ByVal RecognitionType As SpeechRecognitionType, _
                                    ByVal Result As ISpeechRecoResult _
                                    )
    Dim strText As String
    strText = Result.PhraseInfo.GetText
    Debug.Print "Recognition: " & strText & ", " & _
        StreamNumber & ", " & StreamPosition
    
    ' Append the new text to the text box, and add a space at the end of the
    ' text so that it looks better
    txtSpeech.SelStart = m_cChars
    txtSpeech.SelText = strText & " "
    m_cChars = m_cChars + 1 + Len(strText)
End Sub

' This function handles the state of Start and Stop buttons according to
' whether dictation is running.
Private Sub SetState(ByVal bNewState As Boolean)
    m_bRecoRunning = bNewState
    btnStart.Enabled = Not m_bRecoRunning
    btnStop.Enabled = m_bRecoRunning
End Sub
