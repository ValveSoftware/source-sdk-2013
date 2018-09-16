//=============================================================================
//
// Copyright (c) Microsoft Corporation All Rights Reserved.
//
//=============================================================================

namespace SimpleTTS
{
    using System;
    using System.Windows.Forms;
    using System.Threading;
    using SpeechLib;

    /// <summary>
    ///     This form shows how TTS speak works. 
    /// </summary>
    public class MainForm : System.Windows.Forms.Form
    {
        private System.Windows.Forms.CheckBox chkSaveToWavFile;
        private System.Windows.Forms.Button btnSpeak;
        private System.Windows.Forms.TextBox txtSpeakText;
        private System.Windows.Forms.Button btnExit;
        private System.ComponentModel.IContainer components;

        public MainForm()
        {
            //
            // Required for Windows Form Designer support
            //
            InitializeComponent();
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
            this.btnSpeak = new System.Windows.Forms.Button();
            this.chkSaveToWavFile = new System.Windows.Forms.CheckBox();
            this.chkSaveToWavFile.Location = new System.Drawing.Point(8, 80);
            this.chkSaveToWavFile.Size = new System.Drawing.Size(90, 24);

            this.btnExit = new System.Windows.Forms.Button();
            this.txtSpeakText = new System.Windows.Forms.TextBox();
            this.btnSpeak.Location = new System.Drawing.Point(108, 80);
            this.btnSpeak.Size = new System.Drawing.Size(80, 24);
            this.btnSpeak.TabIndex = 4;
            this.btnSpeak.Text = "&Speak";
            this.btnSpeak.Click += new System.EventHandler(this.btnSpeak_Click);
            this.btnExit.Location = new System.Drawing.Point(200, 80);
            this.btnExit.Size = new System.Drawing.Size(80, 24);
            this.btnExit.TabIndex = 2;
            this.btnExit.Text = "&Exit";
            this.btnExit.Click += new System.EventHandler(this.btnExit_Click);
            this.chkSaveToWavFile.Text = "Save to .wav";
            this.txtSpeakText.Location = new System.Drawing.Point(8, 8);
            this.txtSpeakText.Size = new System.Drawing.Size(284, 60);
            this.txtSpeakText.TabIndex = 3;
            this.txtSpeakText.Multiline  = true;

            this.txtSpeakText.Text = "This is Simple TTS application.";
            this.txtSpeakText.WordWrap = true;
            this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
            this.ClientSize = new System.Drawing.Size(299, 120);
            this.Controls.AddRange(new System.Windows.Forms.Control[] {this.btnExit,
                                                                          this.btnSpeak,
                                                                          this.txtSpeakText,
                                                                          this.chkSaveToWavFile});
            this.MaximizeBox = false;
            this.Text = "SimpleTTS";

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

        private void btnSpeak_Click(object sender, System.EventArgs e)
        {
			//Create a TTS voice and speak.
			try 
			{
				SpeechVoiceSpeakFlags SpFlags = SpeechVoiceSpeakFlags.SVSFlagsAsync;
				SpVoice Voice = new SpVoice();
				if (chkSaveToWavFile.Checked)
				{
					SaveFileDialog sfd = new SaveFileDialog();
	    
					sfd.Filter = "All files (*.*)|*.*|wav files (*.wav)|*.wav";
					sfd.Title = "Save to a wave file";
					sfd.FilterIndex = 2;
					sfd.RestoreDirectory = true;
	    
					if (sfd.ShowDialog()== DialogResult.OK) 
					{

						SpeechStreamFileMode SpFileMode = SpeechStreamFileMode.SSFMCreateForWrite;

						SpFileStream SpFileStream = new SpFileStream();
						SpFileStream.Open(sfd.FileName, SpFileMode, false);

						Voice.AudioOutputStream = SpFileStream;
						Voice.Speak(txtSpeakText.Text, SpFlags);
						Voice.WaitUntilDone(Timeout.Infinite);

						SpFileStream.Close();

					}
				}
				else
				{
					Voice.Speak(txtSpeakText.Text, SpFlags);
				}
			}
			catch(Exception error)
			{
				MessageBox.Show("Speak error", "SimpleTTS", MessageBoxButtons.OK, MessageBoxIcon.Error);
			}

        }

        private void btnExit_Click(object sender, System.EventArgs e)
        {
		  this.Dispose();
        }
    }
}
