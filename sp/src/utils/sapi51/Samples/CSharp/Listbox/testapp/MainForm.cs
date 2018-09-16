//=============================================================================
//
// Copyright (c) Microsoft Corporation All Rights Reserved.
//
//=============================================================================

namespace TestApp
{
    using System;
    using System.Drawing;
    using System.Collections;
    using System.ComponentModel;
    using System.Windows.Forms;
    using System.Data;
    using SAPI51Samples;

    /// <summary>
    ///     This form is a simple test application for the user control 
    ///     defined in the SpeechListBox project.
    /// </summary>
    public class MainForm : System.Windows.Forms.Form
    {
        private System.Windows.Forms.CheckBox chkSpeechEnabled;
        private SAPI51Samples.SpeechListBox speechListBox1;
        private System.Windows.Forms.Button btnAdd;
        private System.Windows.Forms.TextBox txtNewItem;
        private System.Windows.Forms.Button btnRemove;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label2;
        private System.ComponentModel.IContainer components = null;

        public MainForm()
        {
            //
            // Required for Windows Form Designer support
            //
            InitializeComponent();
            this.chkSpeechEnabled.Checked = this.speechListBox1.SpeechEnabled;
        }

        /// <summary>
        ///     Clean up any resources being used.
        /// </summary>
        public override void Dispose()
        {
            base.Dispose();
            if(components != null)
                components.Dispose();
        }

        #region Windows Form Designer generated code
        /// <summary>
        ///     Required method for Designer support - do not modify
        ///     the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.txtNewItem = new System.Windows.Forms.TextBox();
            this.btnAdd = new System.Windows.Forms.Button();
            this.label2 = new System.Windows.Forms.Label();
            this.btnRemove = new System.Windows.Forms.Button();
            this.label1 = new System.Windows.Forms.Label();
            this.chkSpeechEnabled = new System.Windows.Forms.CheckBox();
            this.speechListBox1 = new SAPI51Samples.SpeechListBox();
            this.SuspendLayout();
            // 
            // txtNewItem
            // 
            this.txtNewItem.Location = new System.Drawing.Point(86, 225);
            this.txtNewItem.Name = "txtNewItem";
            this.txtNewItem.Size = new System.Drawing.Size(152, 20);
            this.txtNewItem.TabIndex = 5;
            this.txtNewItem.Text = "Seattle";
            this.txtNewItem.TextChanged += new System.EventHandler(this.txtNewItem_TextChanged);
            // 
            // btnAdd
            // 
            this.btnAdd.Location = new System.Drawing.Point(256, 224);
            this.btnAdd.Name = "btnAdd";
            this.btnAdd.Size = new System.Drawing.Size(70, 23);
            this.btnAdd.TabIndex = 6;
            this.btnAdd.Text = "&Add";
            this.btnAdd.Click += new System.EventHandler(this.btnAdd_Click);
            // 
            // label2
            // 
            this.label2.Location = new System.Drawing.Point(8, 2);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(328, 40);
            this.label2.TabIndex = 0;
            this.label2.Text = "Update the speech enabled list box using the Add/Remove button. Then say \"select\"" +
                " + phrase to select an item in the list, e.g. you can say \"select Seattle\" if Se" +
                "attle appears in the list. ";
            // 
            // btnRemove
            // 
            this.btnRemove.Location = new System.Drawing.Point(256, 192);
            this.btnRemove.Name = "btnRemove";
            this.btnRemove.Size = new System.Drawing.Size(70, 23);
            this.btnRemove.TabIndex = 3;
            this.btnRemove.Text = "&Remove";
            this.btnRemove.Click += new System.EventHandler(this.btnRemove_Click);
            // 
            // label1
            // 
            this.label1.Location = new System.Drawing.Point(8, 227);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(78, 16);
            this.label1.TabIndex = 4;
            this.label1.Text = "&Phrase to add:";
            // 
            // chkSpeechEnabled
            // 
            this.chkSpeechEnabled.Location = new System.Drawing.Point(8, 195);
            this.chkSpeechEnabled.Name = "chkSpeechEnabled";
            this.chkSpeechEnabled.Size = new System.Drawing.Size(112, 16);
            this.chkSpeechEnabled.TabIndex = 2;
            this.chkSpeechEnabled.Text = "&Speech Enabled";
            this.chkSpeechEnabled.CheckedChanged += new System.EventHandler(this.chkSpeechEnabled_CheckedChanged);
            // 
            // speechListBox1
            // 
            this.speechListBox1.Location = new System.Drawing.Point(8, 49);
            this.speechListBox1.Name = "speechListBox1";
            this.speechListBox1.Size = new System.Drawing.Size(320, 134);
            this.speechListBox1.SpeechEnabled = true;
            this.speechListBox1.TabIndex = 1;
            // 
            // MainForm
            // 
            this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
            this.ClientSize = new System.Drawing.Size(336, 253);
            this.Controls.AddRange(new System.Windows.Forms.Control[] {
                                                                          this.label2,
                                                                          this.label1,
                                                                          this.btnRemove,
                                                                          this.btnAdd,
                                                                          this.txtNewItem,
                                                                          this.chkSpeechEnabled,
                                                                          this.speechListBox1});
            this.MaximizeBox = false;
            this.Name = "MainForm";
            this.Text = "Speech Enabled ListBox";
            this.ResumeLayout(false);

        }
        #endregion

        /// <summary>
        ///     The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main() 
        {
            Application.Run(new MainForm());
        }

        private void btnAdd_Click(object sender, System.EventArgs e)
        {
            // Add the new item. Internally to SpeechListBox, this will cause 
            // a rebuild of the dynamic grammar used by speech recognition 
            // engine.
            speechListBox1.AddItem(txtNewItem.Text);
            txtNewItem.Text = "";
        }

        private void btnRemove_Click(object sender, System.EventArgs e)
        {
            // Just remove the current selected item. Same as AddItem, removing 
            // an item causes a grammar rebuild as well.
            if( speechListBox1.SelectedIndex >= 0 )
            {
                speechListBox1.RemoveItem(speechListBox1.SelectedIndex);
            }
        }

        private void chkSpeechEnabled_CheckedChanged(object sender, System.EventArgs e)
        {
            speechListBox1.SpeechEnabled = chkSpeechEnabled.Checked;
        }

        private void txtNewItem_TextChanged(object sender, System.EventArgs e)
        {
            // don't allow empty string to be added
            btnAdd.Enabled = txtNewItem.Text != "";
        }
    }
}
