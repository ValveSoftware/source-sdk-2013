//=============================================================================
//
// Copyright (c) Microsoft Corporation All Rights Reserved.
//
//=============================================================================

namespace SAPI51Samples
{
    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.Drawing;
    using System.Data;
    using System.Windows.Forms;
    using System.Diagnostics;
    using SpeechLib;

    /// <summary>
    ///     SpeechListBox is a speech enabled listbox control. It inherits 
    ///     from the standard System.Windows.Forms.ListBox, and thus exposes
    ///     all the standard properties, methods and events. A few new methods
    ///     are added to provide speech specific functionalities.
    /// </summary>
    public class SpeechListBox : System.Windows.Forms.ListBox
    {
        private const int                       grammarId = 10;
        private bool                            speechEnabled = false;
        private bool                            speechInitialized = false;
        private String                          PreCommandString = "Select";
        private SpeechLib.SpSharedRecoContext   objRecoContext;
        private SpeechLib.ISpeechRecoGrammar    grammar;
        private SpeechLib.ISpeechGrammarRule    ruleTopLevel;
        private SpeechLib.ISpeechGrammarRule    ruleListItems;

        /// <summary>
        ///     AddItem is used to add a newitem to the list item collection.
        ///     It will automatically update the grammar. It's equivalent to 
        ///     calling ListBox.Items.Add() followed by BuildGrammar().
        /// </summary>
        /// <param name="newItem">
        ///     The new item to be added to the list.
        /// </param>
        /// <returns>
        ///     The index of the newly added item.
        /// </returns>
        public int AddItem(object newItem)
        {
            // Since we can't add the same word to the same transition in 
            // the grammar, we don't allow same string to be added 
            // multiple times. So do nothing if Item is already in the list. 
            // Some level of error message may be helpful. The sample chooses 
            // to silently ignore to keep code simple.

            // The leading and trailing spaces are not needed, trim it before 
            // inserting. SAPI will return error in AddWordTransition if two 
            // phrases differ only in spaces. A program needs to handle this 
            // error if random phrase is added to a rule.
            // Note: In this sample, we only trim leading and trailing spaces. 
            // Internal spaces will need to be handled as well.

            string  strItem;
            int     index = -1;

            strItem = newItem.ToString().Trim();

            // only add it if trimmed phrase is not empty
            if( strItem.Length > 0 )
            {
                index = this.FindString(strItem);

                if( index < 0 )
                {
                    // if it doesn't exist yet, add it to the list
                    index = this.Items.Add(strItem);

                    // if speech is enabled, we need to update grammar with the change
                    if( speechEnabled ) RebuildGrammar();
                }
            }

            return index;
        }

        /// <summary>
        ///     RemoveItem will remove the item with the given index and then
        ///     call BuildGrammar() to update grammar. It's equivalent to 
        ///     calling ListBox.Items.RemoveAt() followed by BuildGrammar().
        /// </summary>
        /// <param name="index">
        ///     Index of the item to be removed from the list.
        /// </param>
        public void RemoveItem(int index)
        {
            this.Items.RemoveAt(index);
            if( speechEnabled ) RebuildGrammar();
        }

        /// <summary>
        ///     Property SpeechEnabled is read/write-able. When it's set to
        ///     true, speech recognition will be started. When it's set to
        ///     false, speech recognition will be stopped.
        /// </summary>
        public bool SpeechEnabled
        {
            get
            {
                return speechEnabled;
            }
            set
            {
                if( speechEnabled != value )
                {
                    speechEnabled = value ;
                    if(this.DesignMode) return;

                    if (speechEnabled)
                    {
                        EnableSpeech();
                    }
                    else
                    {
                        DisableSpeech();
                    }
                }
            }
        }

        /// <summary>
        ///     RecoContext_Hypothesis is the event handler function for 
        ///     SpSharedRecoContext object's Hypothesis event.
        /// </summary>
        /// <param name="StreamNumber"></param>
        /// <param name="StreamPosition"></param>
        /// <param name="Result"></param>
        /// <remarks>
        ///     See EnableSpeech() for how to hook up this function with the 
        ///     event.
        /// </remarks>
        public void RecoContext_Hypothesis(int StreamNumber, 
            object StreamPosition, 
            ISpeechRecoResult Result)
        {
            Debug.WriteLine("Hypothesis: " + 
                Result.PhraseInfo.GetText(0, -1, true) + ", " +
                StreamNumber + ", " + StreamPosition);
        }

        /// <summary>
        ///     RecoContext_Hypothesis is the event handler function for 
        ///     SpSharedRecoContext object's Recognition event.
        /// </summary>
        /// <param name="StreamNumber"></param>
        /// <param name="StreamPosition"></param>
        /// <param name="RecognitionType"></param>
        /// <param name="Result"></param>
        /// <remarks>
        ///     See EnableSpeech() for how to hook up this function with the 
        ///     event.
        /// </remarks>
        public void RecoContext_Recognition(int StreamNumber, 
            object StreamPosition, 
            SpeechRecognitionType RecognitionType,
            ISpeechRecoResult Result)
        {
            Debug.WriteLine("Recognition: " + 
                Result.PhraseInfo.GetText(0, -1, true) + ", " +
                StreamNumber + ", " + StreamPosition);

            int                     index;
            ISpeechPhraseProperty   oItem;
            
            // oItem will be the property of the second part in the recognized 
            // phase. For example, if the top level rule matchs 
            // "select Seattle". Then the ListItemsRule matches "Seattle" part.
            // The following code will get the property of the "Seattle" 
            // phrase, which is set when the word "Seattle" is added to the 
            // ruleListItems in RebuildGrammar.
            oItem = Result.PhraseInfo.Properties.Item(0).Children.Item(0);
            index = oItem.Id;

            if ((System.Decimal)Result.PhraseInfo.GrammarId == grammarId)
            {
                // Check to see if the item at the same position in the list 
                // still has the same text.
                // This is to prevent the rare case that the user keeps 
                // talking while the list is being added or removed. By the 
                // time this event is fired and handled, the list box may have 
                // already changed.
                if( oItem.Name.CompareTo(this.Items[index].ToString())==0 )
                {
                    this.SelectedIndex = index;
                }
            }
        }

        /// <summary>
        ///     This function will create the main SpSharedRecoContext object 
        ///     and other required objects like Grammar and rules. 
        ///     In this sample, we are building grammar dynamically since 
        ///     listbox content can change from time to time.
        ///     If your grammar is static, you can write your grammar file 
        ///     and ask SAPI to load it during run time. This can reduce the 
        ///     complexity of your code.
        /// </summary>
        private void InitializeSpeech()
        {
            Debug.WriteLine("Initializing SAPI objects...");

            try
            {
                // First of all, let's create the main reco context object. 
                // In this sample, we are using shared reco context. Inproc reco
                // context is also available. Please see the document to decide
                // which is best for your application.
                objRecoContext = new SpeechLib.SpSharedRecoContext();

                // Then, let's set up the event handler. We only care about
                // Hypothesis and Recognition events in this sample.
                objRecoContext.Hypothesis += new 
                    _ISpeechRecoContextEvents_HypothesisEventHandler(
                    RecoContext_Hypothesis);

                objRecoContext.Recognition += new 
                    _ISpeechRecoContextEvents_RecognitionEventHandler(
                    RecoContext_Recognition);

                // Now let's build the grammar.
                // The top level rule consists of two parts: "select <items>". 
                // So we first add a word transition for the "select" part, then 
                // a rule transition for the "<items>" part, which is dynamically 
                // built as items are added or removed from the listbox.
                grammar = objRecoContext.CreateGrammar(grammarId);
                ruleTopLevel = grammar.Rules.Add("TopLevelRule", 
                    SpeechRuleAttributes.SRATopLevel | SpeechRuleAttributes.SRADynamic, 1);
                ruleListItems = grammar.Rules.Add("ListItemsRule", 
                    SpeechRuleAttributes.SRADynamic, 2);

                SpeechLib.ISpeechGrammarRuleState   stateAfterSelect;
                stateAfterSelect = ruleTopLevel.AddState();

                object      PropValue = "";
                ruleTopLevel.InitialState.AddWordTransition(stateAfterSelect,
                    PreCommandString, " ", SpeechGrammarWordType.SGLexical,
                    "", 0, ref PropValue, 1.0F );

                PropValue = "";
                stateAfterSelect.AddRuleTransition(null, ruleListItems, "", 
                    1, ref PropValue, 0F);

                // Now add existing list items to the ruleListItems
                RebuildGrammar();

                // Now we can activate the top level rule. In this sample, only 
                // the top level rule needs to activated. The ListItemsRule is 
                // referenced by the top level rule.
                grammar.CmdSetRuleState("TopLevelRule", SpeechRuleState.SGDSActive);
                speechInitialized = true;
            }
            catch(Exception e)
            {
                System.Windows.Forms.MessageBox.Show(
                    "Exception caught when initializing SAPI." 
                    + " This application may not run correctly.\r\n\r\n"
                    + e.ToString(),
                    "Error");
            }
        }

        /// <summary>
        ///     EnableSpeech will initialize all speech objects on first time,
        ///     then rebuild grammar and start speech recognition.
        /// </summary>
        /// <returns>
        ///     true if speech is enabled and grammar updated.
        ///     false otherwise, which happens if we are in design mode.
        /// </returns>
        /// <remarks>
        ///     This is a private function.
        /// </remarks>
        private bool EnableSpeech()
        {
            Debug.Assert(speechEnabled, "speechEnabled must be true in EnableSpeech");

            if (this.DesignMode) return false;

            if (speechInitialized == false)
            {
                InitializeSpeech();
            }
            else
            {
                RebuildGrammar();
            }

            objRecoContext.State = SpeechRecoContextState.SRCS_Enabled;
            return true;
        }

        /// <summary>
        ///     RebuildGrammar() will update grammar object with current list 
        ///     items. It is called automatically by AddItem and RemoveItem.
        /// </summary>
        /// <returns>
        ///     true if grammar is updated.
        ///     false if grammar is not updated, which can happen if speech is 
        ///     not enabled or if it's in design mode.
        /// </returns>
        /// <remarks>
        ///     RebuildGrammar should be called every time after the list item 
        ///     has changed. AddItem and RemoveItem methods are provided as a 
        ///     way to update list item and the grammar object automatically.
        ///     Don't forget to call RebuildGrammar if the list is changed 
        ///     through ListBox.Items collection. Otherwise speech engine will 
        ///     continue to recognize old list items.
        /// </remarks>
        public bool RebuildGrammar()
        {
            if( !speechEnabled || this.DesignMode )
            {
                return false;
            }

            // In this funtion, we are only rebuilding the ruleListItems, as 
            // this is the only part that's really changing dynamically in 
            // this sample. However, you still have to call 
            // Grammar.Rules.Commit to commit the grammar.
            int             i, count;
            String          word;
            object          propValue = "";

            try
            {
                ruleListItems.Clear();
                count = this.Items.Count;

                for(i=0; i<count; i++)
                {
                    word = this.Items[i].ToString();

                    // Note: if the same word is added more than once to the same 
                    // rule state, SAPI will return error. In this sample, we 
                    // don't allow identical items in the list box so no need for 
                    // the checking, otherwise special checking for identical words
                    // would have to be done here.
                    ruleListItems.InitialState.AddWordTransition(null, word, 
                        " ", SpeechGrammarWordType.SGLexical, word, i, ref propValue, 1F);
                }

                grammar.Rules.Commit();
            }
            catch(Exception e)
            {
                System.Windows.Forms.MessageBox.Show(
                    "Exception caught when rebuilding dynamic listbox rule.\r\n\r\n" 
                    + e.ToString(),
                    "Error");
            }

            return true;
        }


        /// <summary>
        ///     This is a private function that stops speech recognition.
        /// </summary>
        /// <returns></returns>
        private bool DisableSpeech()
        {
            if (this.DesignMode) return false;

            Debug.Assert(speechInitialized, 
                         "speech must be initialized in DisableSpeech");

            if( speechInitialized ) 
            {
                // Putting the recognition context to disabled state will 
                // stop speech recognition. Changing the state to enabled 
                // will start recognition again.
                objRecoContext.State = SpeechRecoContextState.SRCS_Disabled;
            }

            return true;
        }

    }
}

