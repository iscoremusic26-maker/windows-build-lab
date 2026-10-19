namespace DevfMap;
internal static class Program{
 [STAThread] static int Main(string[] args){ApplicationConfiguration.Initialize();
  if(args.Length==4&&args[0]=="--self-test"){try{CompatibilityTests.Run(args[1],args[2]);ImporterTests.Run(args[1],args[2]);MultiTests.Run(args[1],args[2]);Cp13Tests.Run(args[1],args[2]);Cp20Tests.Run(args[3],args[2]);return 0;}catch(Exception e){Directory.CreateDirectory(args[2]);File.WriteAllText(Path.Combine(args[2],"FAIL.txt"),e.ToString());return 1;}}
  Application.Run(new MultiEditor());return 0;
 }
}
