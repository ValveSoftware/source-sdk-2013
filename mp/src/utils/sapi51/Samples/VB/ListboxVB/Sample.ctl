VERSION 5.00
Begin VB.UserControl Sample 
   ClientHeight    =   690
   ClientLeft      =   0
   ClientTop       =   0
   ClientWidth     =   1815
   ScaleHeight     =   690
   ScaleWidth      =   1815
   Begin VB.ListBox InnerList 
      Height          =   450
      Left            =   0
      TabIndex        =   0
      Top             =   0
      Width           =   1575
   End
End
Attribute VB_Name = "Sample"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = True
Option Explicit

' See UserControl_Resize() for how iLevelInResize is used.
' It's needed to make sure our control resizes correctly.
Dim iLevelInResize As Integer

' declare all speech related variables
Const m_GrammarId = 10
Dim bSpeechInitialized As Boolean
Dim WithEvents RecoContext As SpSharedRecoContext
Attribute RecoContext.VB_VarHelpID = -1
Dim Grammar As ISpeechRecoGrammar
Dim TopRule As ISpeechGrammarRule
Dim ListItemsRule As ISpeechGrammarRule

'Event Declarations:
Event ItemCheck(Item As Integer) 'MappingInfo=InnerList,InnerList,-1,ItemCheck
Attribute ItemCheck.VB_Description = "Occurs when a ListBox control's Style property is set to 1 (checkboxes) and an item's checkbox in the ListBox control is selected or cleared."
Event OLEStartDrag(Data As DataObject, AllowedEffects As Long) 'MappingInfo=InnerList,InnerList,-1,OLEStartDrag
Attribute OLEStartDrag.VB_Description = "Occurs when an OLE drag/drop operation is initiated either manually or automatically."
Event OLESetData(Data As DataObject, DataFormat As Integer) 'MappingInfo=InnerList,InnerList,-1,OLESetData
Attribute OLESetData.VB_Description = "Occurs at the OLE drag/drop source control when the drop target requests data that was not provided to the DataObject during the OLEDragStart event."
Event OLEGiveFeedback(Effect As Long, DefaultCursors As Boolean) 'MappingInfo=InnerList,InnerList,-1,OLEGiveFeedback
Attribute OLEGiveFeedback.VB_Description = "Occurs at the source control of an OLE drag/drop operation when the mouse cursor needs to be changed."
Event OLEDragOver(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single, State As Integer) 'MappingInfo=InnerList,InnerList,-1,OLEDragOver
Attribute OLEDragOver.VB_Description = "Occurs when the mouse is moved over the control during an OLE drag/drop operation, if its OLEDropMode property is set to manual."
Event OLEDragDrop(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single) 'MappingInfo=InnerList,InnerList,-1,OLEDragDrop
Attribute OLEDragDrop.VB_Description = "Occurs when data is dropped onto the control via an OLE drag/drop operation, and OLEDropMode is set to manual."
Event OLECompleteDrag(Effect As Long) 'MappingInfo=InnerList,InnerList,-1,OLECompleteDrag
Attribute OLECompleteDrag.VB_Description = "Occurs at the OLE drag/drop source control after a manual or automatic drag/drop has been completed or canceled."
Event Scroll() 'MappingInfo=InnerList,InnerList,-1,Scroll
Attribute Scroll.VB_Description = "Occurs when you reposition the scroll box on a control."
Event Validate(Cancel As Boolean) 'MappingInfo=InnerList,InnerList,-1,Validate
Attribute Validate.VB_Description = "Occurs when a control loses focus to a control that causes validation."
'Default Property Values:
Const m_def_PreCommandString = "Select"
Const m_def_SpeechEnabled = True
'Property Variables:
Dim m_PreCommandString As String
Dim m_SpeechEnabled As Boolean


Private Sub InitializeSpeech()
    ' This function will create the main SpSharedRecoContext object and other
    ' required objects like Grammar and rules. In this sample, we are building
    ' grammar dynamically since listbox content can change from time to time.
    ' If your grammar is static, you can write your grammar file and ask SAPI
    ' to load it during run time. This can reduce the complexity of your code.
    
    On Error GoTo ErrorHandler
    
    If Not bSpeechInitialized Then
        Debug.Print "Initializing speech"
        
        Dim AfterCmdState As ISpeechGrammarRuleState
        Set RecoContext = New SpSharedRecoContext
        Set Grammar = RecoContext.CreateGrammar(m_GrammarId)
        
        ' Add two rules. The top level rule will reference the items rule.
        Set TopRule = Grammar.Rules.Add("TopLevelRule", SRATopLevel Or SRADynamic, 1)
        Set ListItemsRule = Grammar.Rules.Add("ListItemsRule", SRADynamic, 2)
        
        Set AfterCmdState = TopRule.AddState
        
        ' The top level rule consists of two parts: "select <items>". So we first
        ' add a word transition for the "select" part, then a rule transition
        ' for the "<items>" part, which is dynamically built as items are added
        ' or removed from the listbox.
        TopRule.InitialState.AddWordTransition AfterCmdState, _
            m_PreCommandString, " ", , "", 0, 0
        AfterCmdState.AddRuleTransition Nothing, ListItemsRule, "", 1, 1
        
        ' Now add existing list items to the ListItemsRule
        RebuildGrammar
        
        ' Now we can activate the top level rule. In this sample, only the top
        ' level rule needs to activated. The ListItemsRule is referenced by
        ' the top level rule.
        Grammar.CmdSetRuleState "TopLevelRule", SGDSActive
        
        bSpeechInitialized = True
    End If
    
    Exit Sub
    
ErrorHandler:
    MsgBox "SAPI failed to initialize. This application may not run correctly."
End Sub

Friend Sub EnableSpeech()
    Debug.Print "Enabling speech"
    If Not bSpeechInitialized Then Call InitializeSpeech
    
    ' once all objects are initialized, we need to update grammar
    RebuildGrammar
    RecoContext.State = SRCS_Enabled
End Sub

Friend Sub DisableSpeech()
    Debug.Print "Disabling speech"
    
    ' Putting the recognition context to disabled state will stop speech
    ' recognition. Changing the state to enabled will start recognition again.
    If bSpeechInitialized Then RecoContext.State = SRCS_Disabled
End Sub


Private Sub RebuildGrammar()
    ' In this funtion, we are only rebuilding the ListItemRule, as this is the
    ' only part that's really changing dynamically in this sample. However,
    ' you still have to call Grammar.Rules.Commit to commit the grammar.
    
    On Error GoTo ErrorHandler
    
    ' First, clear the rule
    ListItemsRule.Clear
    
    ' Now, add all items to the rule
    Dim i As Integer
    For i = 0 To InnerList.ListCount - 1
        Dim text As String
        text = InnerList.List(i)
        
        ' Note: if the same word is added more than once to the same rule state,
        ' SAPI will return error. In this sample, we don't allow identical items
        ' in the list box so no need for the checking, otherwise special checking
        ' for identical words would have to be done here.
        ListItemsRule.InitialState.AddWordTransition Nothing, text, " ", , text, i, i
    Next
    
    Grammar.Rules.Commit
    Exit Sub
    
ErrorHandler:
    MsgBox "Error when rebuiling dynamic list box grammar: " & Err.Number
End Sub


Private Sub RecoContext_Hypothesis(ByVal StreamNumber As Long, _
                                   ByVal StreamPosition As Variant, _
                                   ByVal Result As ISpeechRecoResult _
                                   )
    
    ' This event is fired when the recognizer thinks there's possible
    ' recognitions.
    Debug.Print "Hypothesis: " & Result.PhraseInfo.GetText & ", " & _
        StreamNumber & ", " & StreamPosition
End Sub

Private Sub RecoContext_Recognition(ByVal StreamNumber As Long, _
                                    ByVal StreamPosition As Variant, _
                                    ByVal RecognitionType As SpeechRecognitionType, _
                                    ByVal Result As ISpeechRecoResult _
                                    )
    
    ' This event is fired when something in the grammar is recognized.
    Debug.Print "Recognition: " & Result.PhraseInfo.GetText & ", " & _
        StreamNumber & ", " & StreamPosition
    
    Dim index As Integer
    Dim oItem As ISpeechPhraseProperty
    
    ' oItem will be the property of the second part in the recognized phase.
    ' For example, if the top level rule matchs "select Seattle". Then the
    ' ListItemsRule matches "Seattle" part. The following code will get the
    ' property of the "Seattle" phrase, which is set when the word "Seattle"
    ' is added to the ListItemsRule in RebuildGrammar.
    Set oItem = Result.PhraseInfo.Properties(1).Children(0)
    index = oItem.Id
    
    If Result.PhraseInfo.GrammarId = m_GrammarId Then
    
        ' Check to see if the item at the same position in the list still has the
        ' same text.
        ' This is to prevent the rare case that the user keeps talking while
        ' the list is being added or removed. By the time this event is fired
        ' and handled, the list box may have already changed.
        If oItem.Name = InnerList.List(index) Then
            InnerList.ListIndex = index
        End If
    End If
End Sub


Private Sub UserControl_Initialize()
    iLevelInResize = 0
    bSpeechInitialized = False
End Sub

Private Sub UserControl_Resize()
    
    ' When the user control is resized, the inner listbox has to be resized
    ' so that it takes up all the area.
    ' Since height of inner ListBox changes by the height of a line of text,
    ' we have to adjust the user control's size, which may cause reentrance to
    ' this Resize() function. iLevelInResize is used to prevent infinite loop.
    iLevelInResize = iLevelInResize + 1
    
    If iLevelInResize = 1 Then
        InnerList.Move 0, 0, Width, Height
        
        ' The following lines will cause Resize events and thus re-entrance
        ' to this function. Since iLevelInResize will not be 1 during
        ' re-entrance, we prevented infinite loop.
        Height = InnerList.Height
        Width = InnerList.Width
    End If
    
    iLevelInResize = iLevelInResize - 1
End Sub

'WARNING! DO NOT REMOVE OR MODIFY THE FOLLOWING COMMENTED LINES!
'MappingInfo=InnerList,InnerList,-1,AddItem
Public Sub AddItem(ByVal Item As String, Optional ByVal index As Variant)
Attribute AddItem.VB_Description = "Adds an item to a ListBox or ComboBox control or a row to a Grid control."

    ' Since we can't add the same word to the same transition in the grammar,
    ' we don't allow same string to be added multiple times.
    ' So do nothing if Item is already in the list. Some level of error
    ' message may be helpful. The sample chooses to silently ignore to keep
    ' code simple.
    
    ' The leading and trailing spaces are not needed, trim it before inserting.
    ' SAPI will return error in AddWordTransition if two phrases differ only
    ' in spaces. A program needs to handle this error if random phrase is
    ' added to a rule.
    ' Note: In this sample, we only trim leading and trailing spaces. Internal
    ' spaces will need to be handled as well.
    
    Item = Trim(Item)
    
    If Item = "" Then
        Exit Sub
    End If
    
    If InnerList.ListCount > 0 Then
        Dim i As Integer
        For i = 0 To InnerList.ListCount - 1
            If StrComp(Item, InnerList.List(i), vbTextCompare) = 0 Then
                Exit Sub
            End If
        Next
    End If

    ' if it doesn't exist yet, add it to the list
    InnerList.AddItem Item, index
    
    ' if speech is enabled, we need to update the grammar with new changes
    If m_SpeechEnabled Then RebuildGrammar
End Sub

'WARNING! DO NOT REMOVE OR MODIFY THE FOLLOWING COMMENTED LINES!
'MappingInfo=InnerList,InnerList,-1,Appearance
Public Property Get Appearance() As Integer
Attribute Appearance.VB_Description = "Returns/sets whether or not an object is painted at run time with 3-D effects."
Attribute Appearance.VB_ProcData.VB_Invoke_Property = ";Appearance"
Attribute Appearance.VB_UserMemId = -520
    Appearance = InnerList.Appearance
End Property

Public Property Let Appearance(ByVal New_Appearance As Integer)
    InnerList.Appearance() = New_Appearance
    PropertyChanged "Appearance"
End Property

'WARNING! DO NOT REMOVE OR MODIFY THE FOLLOWING COMMENTED LINES!
'MappingInfo=InnerList,InnerList,-1,BackColor
Public Property Get BackColor() As OLE_COLOR
Attribute BackColor.VB_Description = "Returns/sets the background color used to display text and graphics in an object."
Attribute BackColor.VB_ProcData.VB_Invoke_Property = ";Appearance"
    BackColor = InnerList.BackColor
End Property

Public Property Let BackColor(ByVal New_BackColor As OLE_COLOR)
    InnerList.BackColor() = New_BackColor
    PropertyChanged "BackColor"
End Property

'WARNING! DO NOT REMOVE OR MODIFY THE FOLLOWING COMMENTED LINES!
'MappingInfo=InnerList,InnerList,-1,CausesValidation
Public Property Get CausesValidation() As Boolean
Attribute CausesValidation.VB_Description = "Returns/sets whether validation occurs on the control which lost focus."
Attribute CausesValidation.VB_ProcData.VB_Invoke_Property = ";Behavior"
    CausesValidation = InnerList.CausesValidation
End Property

Public Property Let CausesValidation(ByVal New_CausesValidation As Boolean)
    InnerList.CausesValidation() = New_CausesValidation
    PropertyChanged "CausesValidation"
End Property

'WARNING! DO NOT REMOVE OR MODIFY THE FOLLOWING COMMENTED LINES!
'MappingInfo=InnerList,InnerList,-1,Clear
Public Sub Clear()
Attribute Clear.VB_Description = "Clears the contents of a control or the system Clipboard."
    InnerList.Clear
End Sub


'WARNING! DO NOT REMOVE OR MODIFY THE FOLLOWING COMMENTED LINES!
'MappingInfo=InnerList,InnerList,-1,Columns
Public Property Get Columns() As Integer
Attribute Columns.VB_Description = "Returns/sets a value that determines whether a ListBox scrolls vertically in a single column (value of 0) or horizontally in snaking columns (values greater than 0)."
    Columns = InnerList.Columns
End Property

Public Property Let Columns(ByVal New_Columns As Integer)
    InnerList.Columns() = New_Columns
    PropertyChanged "Columns"
End Property

'WARNING! DO NOT REMOVE OR MODIFY THE FOLLOWING COMMENTED LINES!
'MappingInfo=InnerList,InnerList,-1,DataMember
Public Property Get DataMember() As String
Attribute DataMember.VB_Description = "Returns/sets a value that describes the DataMember for a data connection."
Attribute DataMember.VB_ProcData.VB_Invoke_Property = ";Data"
    DataMember = InnerList.DataMember
End Property

Public Property Let DataMember(ByVal New_DataMember As String)
    InnerList.DataMember() = New_DataMember
    PropertyChanged "DataMember"
End Property

'WARNING! DO NOT REMOVE OR MODIFY THE FOLLOWING COMMENTED LINES!
'MappingInfo=InnerList,InnerList,-1,DataSource
Public Property Get DataSource() As DataSource
Attribute DataSource.VB_Description = "Sets a value that specifies the Data control through which the current control is bound to a database. "
Attribute DataSource.VB_ProcData.VB_Invoke_Property = ";Data"
    Set DataSource = InnerList.DataSource
End Property

Public Property Set DataSource(ByVal New_DataSource As DataSource)
    Set InnerList.DataSource = New_DataSource
    PropertyChanged "DataSource"
End Property

'WARNING! DO NOT REMOVE OR MODIFY THE FOLLOWING COMMENTED LINES!
'MappingInfo=InnerList,InnerList,-1,Enabled
Public Property Get Enabled() As Boolean
Attribute Enabled.VB_Description = "Returns/sets a value that determines whether an object can respond to user-generated events."
Attribute Enabled.VB_ProcData.VB_Invoke_Property = ";Behavior"
    Enabled = InnerList.Enabled
End Property

Public Property Let Enabled(ByVal New_Enabled As Boolean)
    InnerList.Enabled() = New_Enabled
    PropertyChanged "Enabled"
End Property

'WARNING! DO NOT REMOVE OR MODIFY THE FOLLOWING COMMENTED LINES!
'MappingInfo=InnerList,InnerList,-1,FontUnderline
Public Property Get FontUnderline() As Boolean
Attribute FontUnderline.VB_Description = "Returns/sets underline font styles."
Attribute FontUnderline.VB_MemberFlags = "400"
    FontUnderline = InnerList.FontUnderline
End Property

Public Property Let FontUnderline(ByVal New_FontUnderline As Boolean)
    InnerList.FontUnderline() = New_FontUnderline
End Property

'WARNING! DO NOT REMOVE OR MODIFY THE FOLLOWING COMMENTED LINES!
'MappingInfo=InnerList,InnerList,-1,FontStrikethru
Public Property Get FontStrikethru() As Boolean
Attribute FontStrikethru.VB_Description = "Returns/sets strikethrough font styles."
Attribute FontStrikethru.VB_MemberFlags = "400"
    FontStrikethru = InnerList.FontStrikethru
End Property

Public Property Let FontStrikethru(ByVal New_FontStrikethru As Boolean)
    InnerList.FontStrikethru() = New_FontStrikethru
End Property

'WARNING! DO NOT REMOVE OR MODIFY THE FOLLOWING COMMENTED LINES!
'MappingInfo=InnerList,InnerList,-1,FontSize
Public Property Get FontSize() As Single
Attribute FontSize.VB_Description = "Specifies the size (in points) of the font that appears in each row for the given level."
Attribute FontSize.VB_MemberFlags = "400"
    FontSize = InnerList.FontSize
End Property

Public Property Let FontSize(ByVal New_FontSize As Single)
    InnerList.FontSize() = New_FontSize
End Property

'WARNING! DO NOT REMOVE OR MODIFY THE FOLLOWING COMMENTED LINES!
'MappingInfo=InnerList,InnerList,-1,FontName
Public Property Get FontName() As String
Attribute FontName.VB_Description = "Specifies the name of the font that appears in each row for the given level."
Attribute FontName.VB_MemberFlags = "400"
    FontName = InnerList.FontName
End Property

Public Property Let FontName(ByVal New_FontName As String)
    InnerList.FontName() = New_FontName
End Property

'WARNING! DO NOT REMOVE OR MODIFY THE FOLLOWING COMMENTED LINES!
'MappingInfo=InnerList,InnerList,-1,FontItalic
Public Property Get FontItalic() As Boolean
Attribute FontItalic.VB_Description = "Returns/sets italic font styles."
Attribute FontItalic.VB_MemberFlags = "400"
    FontItalic = InnerList.FontItalic
End Property

Public Property Let FontItalic(ByVal New_FontItalic As Boolean)
    InnerList.FontItalic() = New_FontItalic
End Property

'WARNING! DO NOT REMOVE OR MODIFY THE FOLLOWING COMMENTED LINES!
'MappingInfo=InnerList,InnerList,-1,FontBold
Public Property Get FontBold() As Boolean
Attribute FontBold.VB_Description = "Returns/sets bold font styles."
Attribute FontBold.VB_MemberFlags = "400"
    FontBold = InnerList.FontBold
End Property

Public Property Let FontBold(ByVal New_FontBold As Boolean)
    InnerList.FontBold() = New_FontBold
End Property

'WARNING! DO NOT REMOVE OR MODIFY THE FOLLOWING COMMENTED LINES!
'MappingInfo=InnerList,InnerList,-1,Font
Public Property Get Font() As Font
Attribute Font.VB_Description = "Returns a Font object."
Attribute Font.VB_ProcData.VB_Invoke_Property = ";Font"
Attribute Font.VB_UserMemId = -512
    Set Font = InnerList.Font
End Property

Public Property Set Font(ByVal New_Font As Font)
    Set InnerList.Font = New_Font
    PropertyChanged "Font"
End Property

'WARNING! DO NOT REMOVE OR MODIFY THE FOLLOWING COMMENTED LINES!
'MappingInfo=InnerList,InnerList,-1,ForeColor
Public Property Get ForeColor() As OLE_COLOR
Attribute ForeColor.VB_Description = "Returns/sets the foreground color used to display text and graphics in an object."
Attribute ForeColor.VB_ProcData.VB_Invoke_Property = ";Appearance"
    ForeColor = InnerList.ForeColor
End Property

Public Property Let ForeColor(ByVal New_ForeColor As OLE_COLOR)
    InnerList.ForeColor() = New_ForeColor
    PropertyChanged "ForeColor"
End Property

'WARNING! DO NOT REMOVE OR MODIFY THE FOLLOWING COMMENTED LINES!
'MappingInfo=InnerList,InnerList,-1,hWnd
Public Property Get hWnd() As Long
Attribute hWnd.VB_Description = "Returns a handle (from Microsoft Windows) to an object's window."
    hWnd = InnerList.hWnd
End Property

'WARNING! DO NOT REMOVE OR MODIFY THE FOLLOWING COMMENTED LINES!
'MappingInfo=InnerList,InnerList,-1,IntegralHeight
Public Property Get IntegralHeight() As Boolean
Attribute IntegralHeight.VB_Description = "Returns/Sets a value indicating whether the control displays partial items."
Attribute IntegralHeight.VB_ProcData.VB_Invoke_Property = ";List"
    IntegralHeight = InnerList.IntegralHeight
End Property

Private Sub InnerList_ItemCheck(Item As Integer)
    RaiseEvent ItemCheck(Item)
End Sub

'WARNING! DO NOT REMOVE OR MODIFY THE FOLLOWING COMMENTED LINES!
'MappingInfo=InnerList,InnerList,-1,ItemData
Public Property Get ItemData(ByVal index As Integer) As Long
Attribute ItemData.VB_Description = "Returns/sets a specific number for each item in a ComboBox or ListBox control."
Attribute ItemData.VB_ProcData.VB_Invoke_Property = ";List"
    ItemData = InnerList.ItemData(index)
End Property

Public Property Let ItemData(ByVal index As Integer, ByVal New_ItemData As Long)
    InnerList.ItemData(index) = New_ItemData
    PropertyChanged "ItemData"
End Property


'WARNING! DO NOT REMOVE OR MODIFY THE FOLLOWING COMMENTED LINES!
'MappingInfo=InnerList,InnerList,-1,ListIndex
Public Property Get ListIndex() As Integer
Attribute ListIndex.VB_Description = "Returns/sets the index of the currently selected item in the control."
Attribute ListIndex.VB_MemberFlags = "400"
    ListIndex = InnerList.ListIndex
End Property

Public Property Let ListIndex(ByVal New_ListIndex As Integer)
    InnerList.ListIndex() = New_ListIndex
End Property

'WARNING! DO NOT REMOVE OR MODIFY THE FOLLOWING COMMENTED LINES!
'MappingInfo=InnerList,InnerList,-1,ListCount
Public Property Get ListCount() As Integer
Attribute ListCount.VB_Description = "Returns the number of items in the list portion of a control."
    ListCount = InnerList.ListCount
End Property

'WARNING! DO NOT REMOVE OR MODIFY THE FOLLOWING COMMENTED LINES!
'MappingInfo=InnerList,InnerList,-1,List
Public Property Get List(ByVal index As Integer) As String
Attribute List.VB_Description = "Returns/sets the items contained in a control's list portion."
Attribute List.VB_ProcData.VB_Invoke_Property = ";List"
Attribute List.VB_UserMemId = 0
    List = InnerList.List(index)
End Property

Public Property Let List(ByVal index As Integer, ByVal New_List As String)
    InnerList.List(index) = New_List
    PropertyChanged "List"
End Property

'WARNING! DO NOT REMOVE OR MODIFY THE FOLLOWING COMMENTED LINES!
'MappingInfo=InnerList,InnerList,-1,MousePointer
Public Property Get MousePointer() As Integer
Attribute MousePointer.VB_Description = "Returns/sets the type of mouse pointer displayed when over part of an object."
    MousePointer = InnerList.MousePointer
End Property

Public Property Let MousePointer(ByVal New_MousePointer As Integer)
    InnerList.MousePointer() = New_MousePointer
    PropertyChanged "MousePointer"
End Property


'WARNING! DO NOT REMOVE OR MODIFY THE FOLLOWING COMMENTED LINES!
'MappingInfo=InnerList,InnerList,-1,MouseIcon
Public Property Get MouseIcon() As Picture
Attribute MouseIcon.VB_Description = "Sets a custom mouse icon."
    Set MouseIcon = InnerList.MouseIcon
End Property

Public Property Set MouseIcon(ByVal New_MouseIcon As Picture)
    Set InnerList.MouseIcon = New_MouseIcon
    PropertyChanged "MouseIcon"
End Property

'WARNING! DO NOT REMOVE OR MODIFY THE FOLLOWING COMMENTED LINES!
'MappingInfo=InnerList,InnerList,-1,MultiSelect
Public Property Get MultiSelect() As Integer
Attribute MultiSelect.VB_Description = "Returns/sets a value that determines whether a user can make multiple selections in a control."
    MultiSelect = InnerList.MultiSelect
End Property

'WARNING! DO NOT REMOVE OR MODIFY THE FOLLOWING COMMENTED LINES!
'MappingInfo=InnerList,InnerList,-1,NewIndex
Public Property Get NewIndex() As Integer
Attribute NewIndex.VB_Description = "Returns the index of the item most recently added to a control."
    NewIndex = InnerList.NewIndex
End Property

Private Sub InnerList_OLEStartDrag(Data As DataObject, AllowedEffects As Long)
    RaiseEvent OLEStartDrag(Data, AllowedEffects)
End Sub

Private Sub InnerList_OLESetData(Data As DataObject, DataFormat As Integer)
    RaiseEvent OLESetData(Data, DataFormat)
End Sub

Private Sub InnerList_OLEGiveFeedback(Effect As Long, DefaultCursors As Boolean)
    RaiseEvent OLEGiveFeedback(Effect, DefaultCursors)
End Sub

'WARNING! DO NOT REMOVE OR MODIFY THE FOLLOWING COMMENTED LINES!
'MappingInfo=InnerList,InnerList,-1,OLEDropMode
Public Property Get OLEDropMode() As Integer
Attribute OLEDropMode.VB_Description = "Returns/Sets whether this object can act as an OLE drop target."
Attribute OLEDropMode.VB_ProcData.VB_Invoke_Property = ";Behavior"
    OLEDropMode = InnerList.OLEDropMode
End Property

Public Property Let OLEDropMode(ByVal New_OLEDropMode As Integer)
    InnerList.OLEDropMode() = New_OLEDropMode
    PropertyChanged "OLEDropMode"
End Property

Private Sub InnerList_OLEDragOver(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single, State As Integer)
    RaiseEvent OLEDragOver(Data, Effect, Button, Shift, X, Y, State)
End Sub

'WARNING! DO NOT REMOVE OR MODIFY THE FOLLOWING COMMENTED LINES!
'MappingInfo=InnerList,InnerList,-1,OLEDragMode
Public Property Get OLEDragMode() As Integer
Attribute OLEDragMode.VB_Description = "Returns/Sets whether this object can act as an OLE drag/drop source, and whether this process is started automatically or under programmatic control."
Attribute OLEDragMode.VB_ProcData.VB_Invoke_Property = ";Behavior"
    OLEDragMode = InnerList.OLEDragMode
End Property

Public Property Let OLEDragMode(ByVal New_OLEDragMode As Integer)
    InnerList.OLEDragMode() = New_OLEDragMode
    PropertyChanged "OLEDragMode"
End Property

Private Sub InnerList_OLEDragDrop(Data As DataObject, Effect As Long, Button As Integer, Shift As Integer, X As Single, Y As Single)
    RaiseEvent OLEDragDrop(Data, Effect, Button, Shift, X, Y)
End Sub

'WARNING! DO NOT REMOVE OR MODIFY THE FOLLOWING COMMENTED LINES!
'MappingInfo=InnerList,InnerList,-1,OLEDrag
Public Sub OLEDrag()
Attribute OLEDrag.VB_Description = "Starts an OLE drag/drop event with the given control as the source."
    InnerList.OLEDrag
End Sub

Private Sub InnerList_OLECompleteDrag(Effect As Long)
    RaiseEvent OLECompleteDrag(Effect)
End Sub

'WARNING! DO NOT REMOVE OR MODIFY THE FOLLOWING COMMENTED LINES!
'MappingInfo=InnerList,InnerList,-1,RemoveItem
Public Sub RemoveItem(ByVal index As Integer)
Attribute RemoveItem.VB_Description = "Removes an item from a ListBox or ComboBox control or a row from a Grid control."
    InnerList.RemoveItem index
    If m_SpeechEnabled Then RebuildGrammar
End Sub

'WARNING! DO NOT REMOVE OR MODIFY THE FOLLOWING COMMENTED LINES!
'MappingInfo=InnerList,InnerList,-1,Refresh
Public Sub Refresh()
Attribute Refresh.VB_Description = "Forces a complete repaint of a object."
    InnerList.Refresh
End Sub

'WARNING! DO NOT REMOVE OR MODIFY THE FOLLOWING COMMENTED LINES!
'MappingInfo=InnerList,InnerList,-1,RightToLeft
Public Property Get RightToLeft() As Boolean
Attribute RightToLeft.VB_Description = "Determines text display direction and control visual appearance on a bidirectional system."
Attribute RightToLeft.VB_ProcData.VB_Invoke_Property = ";Behavior"
    RightToLeft = InnerList.RightToLeft
End Property

Public Property Let RightToLeft(ByVal New_RightToLeft As Boolean)
    InnerList.RightToLeft() = New_RightToLeft
    PropertyChanged "RightToLeft"
End Property

Private Sub InnerList_Scroll()
    RaiseEvent Scroll
End Sub

'WARNING! DO NOT REMOVE OR MODIFY THE FOLLOWING COMMENTED LINES!
'MappingInfo=InnerList,InnerList,-1,Selected
Public Property Get Selected(ByVal index As Integer) As Boolean
Attribute Selected.VB_Description = "Returns/sets the selection status of an item in a control."
    Selected = InnerList.Selected(index)
End Property

Public Property Let Selected(ByVal index As Integer, ByVal New_Selected As Boolean)
    InnerList.Selected(index) = New_Selected
End Property

'WARNING! DO NOT REMOVE OR MODIFY THE FOLLOWING COMMENTED LINES!
'MappingInfo=InnerList,InnerList,-1,SelCount
Public Property Get SelCount() As Integer
Attribute SelCount.VB_Description = "Returns the number of selected items in a ListBox control."
    SelCount = InnerList.SelCount
End Property

'WARNING! DO NOT REMOVE OR MODIFY THE FOLLOWING COMMENTED LINES!
'MappingInfo=InnerList,InnerList,-1,Style
Public Property Get Style() As Integer
Attribute Style.VB_Description = "Returns/sets a value that determines whether checkboxes are displayed inside a ListBox control."
Attribute Style.VB_ProcData.VB_Invoke_Property = ";Appearance"
    Style = InnerList.Style
End Property

'WARNING! DO NOT REMOVE OR MODIFY THE FOLLOWING COMMENTED LINES!
'MappingInfo=InnerList,InnerList,-1,Sorted
Public Property Get Sorted() As Boolean
Attribute Sorted.VB_Description = "Indicates whether the elements of a control are automatically sorted alphabetically."
Attribute Sorted.VB_ProcData.VB_Invoke_Property = ";Behavior"
    Sorted = InnerList.Sorted
End Property

'WARNING! DO NOT REMOVE OR MODIFY THE FOLLOWING COMMENTED LINES!
'MappingInfo=InnerList,InnerList,-1,Text
Public Property Get text() As String
Attribute text.VB_Description = "Returns/sets the text contained in the control."
    text = InnerList.text
End Property

Public Property Let text(ByVal New_Text As String)
    InnerList.text() = New_Text
    PropertyChanged "Text"
End Property

'WARNING! DO NOT REMOVE OR MODIFY THE FOLLOWING COMMENTED LINES!
'MappingInfo=InnerList,InnerList,-1,ToolTipText
Public Property Get ToolTipText() As String
Attribute ToolTipText.VB_Description = "Returns/sets the text displayed when the mouse is paused over the control."
    ToolTipText = InnerList.ToolTipText
End Property

Public Property Let ToolTipText(ByVal New_ToolTipText As String)
    InnerList.ToolTipText() = New_ToolTipText
    PropertyChanged "ToolTipText"
End Property

'WARNING! DO NOT REMOVE OR MODIFY THE FOLLOWING COMMENTED LINES!
'MappingInfo=InnerList,InnerList,-1,TopIndex
Public Property Get TopIndex() As Integer
Attribute TopIndex.VB_Description = "Returns/sets which item in a control is displayed in the topmost position."
    TopIndex = InnerList.TopIndex
End Property

Public Property Let TopIndex(ByVal New_TopIndex As Integer)
    InnerList.TopIndex() = New_TopIndex
    PropertyChanged "TopIndex"
End Property

Private Sub InnerList_Validate(Cancel As Boolean)
    RaiseEvent Validate(Cancel)
End Sub

'WARNING! DO NOT REMOVE OR MODIFY THE FOLLOWING COMMENTED LINES!
'MappingInfo=InnerList,InnerList,-1,WhatsThisHelpID
Public Property Get WhatsThisHelpID() As Long
Attribute WhatsThisHelpID.VB_Description = "Returns/sets an associated context number for an object."
    WhatsThisHelpID = InnerList.WhatsThisHelpID
End Property

Public Property Let WhatsThisHelpID(ByVal New_WhatsThisHelpID As Long)
    InnerList.WhatsThisHelpID() = New_WhatsThisHelpID
    PropertyChanged "WhatsThisHelpID"
End Property

'Load property values from storage
Private Sub UserControl_ReadProperties(PropBag As PropertyBag)
    Dim index As Integer
    Dim Count As Integer

    InnerList.Appearance = PropBag.ReadProperty("Appearance", 1)
    InnerList.BackColor = PropBag.ReadProperty("BackColor", &H80000005)
    InnerList.CausesValidation = PropBag.ReadProperty("CausesValidation", True)
    If PropBag.ReadProperty("Columns", 0) <> 0 Then
        InnerList.Columns = PropBag.ReadProperty("Columns", 0)
    End If
    InnerList.DataMember = PropBag.ReadProperty("DataMember", "")
    Set DataSource = PropBag.ReadProperty("DataSource", Nothing)
    InnerList.Enabled = PropBag.ReadProperty("Enabled", True)
    Set InnerList.Font = PropBag.ReadProperty("Font", Ambient.Font)
    InnerList.ForeColor = PropBag.ReadProperty("ForeColor", &H80000008)
    
    Count = PropBag.ReadProperty("ListCount", 0)
    For index = 0 To Count - 1
        InnerList.ItemData(index) = PropBag.ReadProperty("ItemData" & index, 0)
        InnerList.List(index) = PropBag.ReadProperty("List" & index, "")
    Next
    
    InnerList.MousePointer = PropBag.ReadProperty("MousePointer", 0)
    Set MouseIcon = PropBag.ReadProperty("MouseIcon", Nothing)
    InnerList.OLEDropMode = PropBag.ReadProperty("OLEDropMode", 0)
    InnerList.OLEDragMode = PropBag.ReadProperty("OLEDragMode", 0)
    InnerList.RightToLeft = PropBag.ReadProperty("RightToLeft", False)
    InnerList.text = PropBag.ReadProperty("Text", "")
    InnerList.ToolTipText = PropBag.ReadProperty("ToolTipText", "")
    InnerList.TopIndex = PropBag.ReadProperty("TopIndex", 0)
    InnerList.WhatsThisHelpID = PropBag.ReadProperty("WhatsThisHelpID", 0)
    m_PreCommandString = PropBag.ReadProperty("PreCommandString", m_def_PreCommandString)
    Me.SpeechEnabled = PropBag.ReadProperty("SpeechEnabled", m_def_SpeechEnabled)
End Sub

'Write property values to storage
Private Sub UserControl_WriteProperties(PropBag As PropertyBag)
    Dim index As Integer

    Call PropBag.WriteProperty("Appearance", InnerList.Appearance, 1)
    Call PropBag.WriteProperty("BackColor", InnerList.BackColor, &H80000005)
    Call PropBag.WriteProperty("CausesValidation", InnerList.CausesValidation, True)
    Call PropBag.WriteProperty("Columns", InnerList.Columns, 0)
    Call PropBag.WriteProperty("DataMember", InnerList.DataMember, "")
    Call PropBag.WriteProperty("DataSource", DataSource, Nothing)
    Call PropBag.WriteProperty("Enabled", InnerList.Enabled, True)
    Call PropBag.WriteProperty("Font", InnerList.Font, Ambient.Font)
    Call PropBag.WriteProperty("ForeColor", InnerList.ForeColor, &H80000008)

    Call PropBag.WriteProperty("ListCount", InnerList.ListCount, 0)
    For index = 0 To InnerList.ListCount - 1
        Call PropBag.WriteProperty("ItemData" & index, InnerList.ItemData(index), 0)
        Call PropBag.WriteProperty("List" & index, InnerList.List(index), "")
    Next
    
    Call PropBag.WriteProperty("MousePointer", InnerList.MousePointer, 0)
    Call PropBag.WriteProperty("MouseIcon", MouseIcon, Nothing)
    Call PropBag.WriteProperty("OLEDropMode", InnerList.OLEDropMode, 0)
    Call PropBag.WriteProperty("OLEDragMode", InnerList.OLEDragMode, 0)
    Call PropBag.WriteProperty("RightToLeft", InnerList.RightToLeft, False)
    Call PropBag.WriteProperty("Text", InnerList.text, "")
    Call PropBag.WriteProperty("ToolTipText", InnerList.ToolTipText, "")
    Call PropBag.WriteProperty("TopIndex", InnerList.TopIndex, 0)
    Call PropBag.WriteProperty("WhatsThisHelpID", InnerList.WhatsThisHelpID, 0)
    Call PropBag.WriteProperty("SpeechEnabled", m_SpeechEnabled, m_def_SpeechEnabled)
    Call PropBag.WriteProperty("PreCommandString", m_PreCommandString, m_def_PreCommandString)
End Sub

'WARNING! DO NOT REMOVE OR MODIFY THE FOLLOWING COMMENTED LINES!
'MemberInfo=0,0,0,True
Public Property Get SpeechEnabled() As Boolean
Attribute SpeechEnabled.VB_Description = "Whether speech recognition is enabled or not."
    SpeechEnabled = m_SpeechEnabled
End Property

Public Property Let SpeechEnabled(ByVal New_SpeechEnabled As Boolean)
    If m_SpeechEnabled <> New_SpeechEnabled Then
        m_SpeechEnabled = New_SpeechEnabled
        
        If Ambient.UserMode Then
            If m_SpeechEnabled = True Then
                Call EnableSpeech
            Else
                Call DisableSpeech
            End If
        End If
        
        PropertyChanged "SpeechEnabled"
    End If
End Property

'Initialize Properties for User Control
Private Sub UserControl_InitProperties()
    m_PreCommandString = m_def_PreCommandString
    Me.SpeechEnabled = m_def_SpeechEnabled
End Sub

'WARNING! DO NOT REMOVE OR MODIFY THE FOLLOWING COMMENTED LINES!
'MemberInfo=13,1,0,Select
Public Property Get PreCommandString() As String
Attribute PreCommandString.VB_Description = "This property is used to determine what word or words a user needs to say to get the listbox to recognized individual list items."
    PreCommandString = m_PreCommandString
End Property

Public Property Let PreCommandString(ByVal New_PreCommandString As String)
    
    ' This property is not available during run time to simplify sample code.
    ' To support it in run time, you will need to dynamically rebuild the top
    ' level rule when this property changes.
    
    ' If a run time attempt is made to change this property, error is raised.
    If Ambient.UserMode Then Err.Raise 382
    m_PreCommandString = New_PreCommandString
    PropertyChanged "PreCommandString"
End Property

