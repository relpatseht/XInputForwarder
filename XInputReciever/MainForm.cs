using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace XInputReciever
{
    public partial class MainForm : Form
    {
        IntPtr injector = IntPtr.Zero;

        public MainForm()
        {
            InitializeComponent();
            UpdateAppList();
        }
        
        private void UpdateAppList()
        {
            List<Process> processes = Process.GetProcesses().ToList().Where(t => t.MainWindowTitle.Count() > 0).OrderByDescending(t => t.UserProcessorTime).ToList();

            lstApps.DisplayMember = "MainWindowTitle";
            lstApps.DataSource = processes;
        }

        private void btnUpdateApps_Click(object sender, EventArgs e)
        {
            UpdateAppList();
        }

        private void btnConnectApp_Click(object sender, EventArgs e)
        {
            if (lstApps.SelectedIndex >= 0)
            {
                DestroyXInputForwarderInjector(injector);

                int procId = (lstApps.SelectedItem as Process).Id;

                injector = CreateXInputForwarderInjector(procId);

                InjectDLLAndCallFunction(injector, Application.StartupPath, "XInputForwarder.dll", "InitXInputForward");
            }
        }

        [DllImport("XInputForwarderInjector.dll", EntryPoint = "CreateXInputForwarderInjector", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern IntPtr CreateXInputForwarderInjector(int procId);

        [DllImport("XInputForwarderInjector.dll", EntryPoint = "DestroyXInputForwarderInjector", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern void DestroyXInputForwarderInjector(IntPtr injector);

        [DllImport("XInputForwarderInjector.dll", EntryPoint = "InjectDLLAndCallFunction", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        static extern bool InjectDLLAndCallFunction(IntPtr injector,
                                            [MarshalAs(UnmanagedType.LPStr)] String dllDir,
                                            [MarshalAs(UnmanagedType.LPStr)] String dllName,
                                            [MarshalAs(UnmanagedType.LPStr)] String functionName );
    }
}
