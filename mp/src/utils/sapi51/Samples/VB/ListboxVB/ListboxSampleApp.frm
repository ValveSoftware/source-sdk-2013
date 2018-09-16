VERSION 5.00
Object = "*\ASAPI51ListBox.vbp"
Begin VB.Form MainForm 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "Speech Enabled ListBox"
   ClientHeight    =   4215
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   4935
   Icon            =   "ListBoxSampleApp.frx":0000
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   ScaleHeight     =   4215
   ScaleWidth      =   4935
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton cmdRemove 
      Caption         =   "&Remove"
      Height          =   355
      Left            =   3700
      TabIndex        =   3
      Top             =   3200
      Width           =   1100
   End
   Begin VB.CheckBox chkSpeechEnabled 
      Caption         =   "Speech &enabled"
      Height          =   255
      Left            =   120
      TabIndex        =   2
      Top             =   3250
      Width           =   1695
   End
   Begin VB.CommandButton cmdAdd 
      Caption         =   "&Add"
      Height          =   355
      Left            =   3700
      TabIndex        =   6
      Top             =   3720
      Width           =   1100
   End
   Begin VB.TextBox txtNewItem 
      Height          =   315
      Left            =   1320
      TabIndex        =   5
      Text            =   "Seattle"
      Top             =   3740
      Width           =   2175
   End
   Begin SAPI51ListBox.Sample SpeechListBox 
      Height          =   2205
      Left            =   120
      TabIndex        =   1
      Top             =   840
      Width           =   4680
      _ExtentX        =   8255
      _ExtentY        =   3889
      BeginProperty Font {0BE35203-8F91-11CE-9DE3-00AA004BB851} 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
   End
   Begin VB.Label Label2 
      Caption         =   $"ListBoxSampleApp.frx":014A
      Height          =   615
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   4680
   End
   Begin VB.Label Label1 
      Caption         =   "&Phrase to add:"
      Height          =   255
      Left            =   120
      TabIndex        =   4
      Top             =   3770
      Width           =   1040
   End
End
Attribute VB_Name = "MainForm"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
'=============================================================================
'
' This form is simple test application for the user control defined in sample.vbp.
'
' Copyright @ 2001 Microsoft Corporation All Rights Reserved.
'=============================================================================

Private Sub Form_Load()
    If SpeechListBox.SpeechEnabled Then
        chkSpeechEnabled = 1
    Else
        chkSpeechEnabled = 0
    End If
End Sub

Private Sub chkSpeechEnabled_Click()
    SpeechListBox.SpeechEnabled = (chkSpeechEnabled = 1)
End Sub

Private Sub cmdAdd_Click()
    ' Add the new item. Internally to SpeechListBox, this will cause a rebuild
    ' of the dynamic grammar used by speech recognition engine.
    SpeechListBox.AddItem (txtNewItem)
    txtNewItem = ""
End Sub

Private Sub cmdRemove_Click()
    ' Just remove the current selected item. Same as AddItem, removing an item
    ' causes a grammar rebuild as well.
    If SpeechListBox.ListIndex <> -1 Then
        SpeechListBox.RemoveItem SpeechListBox.ListIndex
    End If
End Sub

Private Sub txtNewItem_Change()
    ' Disallow empty item.
    cmdAdd.Enabled = txtNewItem <> ""
End Sub

Private Sub txtNewItem_GotFocus()
    ' When user focuses on the new item box, make the Add button default
    ' so that return key is same as clicking on Add button.
    cmdAdd.Default = True
End Sub
