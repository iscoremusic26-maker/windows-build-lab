using System.Diagnostics;
namespace DevfMap;
public sealed class Canvas:Control {
 public Canvas(){DoubleBuffered=true;ResizeRedraw=true;TabStop=true;BackColor=Color.FromArgb(24,30,38);}
}
public sealed class Editor:Form {
 Document data=Document.Empty();Geometry? geometry;readonly Projection projection=new();readonly Canvas map=new(){Dock=DockStyle.Fill};
 readonly ListBox list=new(){Width=310,Height=240};readonly Label detail=new(){AutoSize=false,Width=310,Height=120};
 readonly Label status=new(){Dock=DockStyle.Bottom,Height=60,Padding=new Padding(10),Text="Importe R03_00.arc legítimo para dibujar la región. No se incluyen recursos del juego."};
 readonly CheckBox enabled=new(){Text="Activo tras reiniciar",Width=280};
 readonly NumericUpDown yaw=new(){Minimum=-32768,Maximum=32767,Width=220};
 readonly ComboBox tool=new(){DropDownStyle=ComboBoxStyle.DropDownList,Width=160};
 string? path,loadedHash;string dataDir=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData),"TwilitRealm","Dusklight","mod_data","dev.bokoblin.expansion");
 bool dirty,updating,dragging;Point last;int moving=-1;
 public int ModelCount=>data.encounters.Count;
 public Editor(){
  Text="DEVF Map Editor CP10.1 0.10.1 · compatibilidad CP08 — TEST privado";Width=1280;Height=860;MinimumSize=new Size(980,700);
  var top=new FlowLayoutPanel{Dock=DockStyle.Top,AutoSize=true,WrapContents=true,Padding=new Padding(6)};
  void Button(string name,Action action){var b=new Button{Text=name,AutoSize=true};b.Click+=(_,_)=>Run(action);top.Controls.Add(b);}
  var zone=new ComboBox{DropDownStyle=ComboBoxStyle.DropDownList,Width=265};zone.Items.Add("Hyrule Field F_SP121 · sala 3 · capa 6");zone.SelectedIndex=0;top.Controls.Add(zone);
  Button("CP10 · edición multi-room QA",()=>new MultiEditor().Show(this));Button("Importar juego / Catálogo",()=>{var browser=new CatalogBrowser(AcceptImported);browser.Show(this);});Button("Importar mapa ARC",Import);Button("Nuevo",New);Button("Abrir JSON",Open);Button("Guardar",Save);Button("Restaurar respaldo",Restore);
  Button("Carpeta de configuración",Folder);Button("Iniciar Dusklight",Launch);Button("Encuadrar",()=>{projection.CenterX=-1250;projection.CenterZ=61000;projection.Scale=Math.Min(map.Width/2200.0,map.Height/4200.0);map.Invalidate();});
  tool.Items.AddRange(new object[]{"Seleccionar / mover","Añadir","Desplazar mapa"});tool.SelectedIndex=0;top.Controls.Add(tool);
  var right=new FlowLayoutPanel{Dock=DockStyle.Right,Width=335,FlowDirection=FlowDirection.TopDown,WrapContents=false,AutoScroll=true,Padding=new Padding(10)};
  right.Controls.Add(new Label{Text="Bokoblin E_OC2 · nativo · máximo 8\nNo modifica HP, apariencia ni IA.",Width=310,Height=45});
  right.Controls.Add(list);right.Controls.Add(enabled);right.Controls.Add(new Label{Text="Orientación yaw s16 (-32768…32767)",AutoSize=true});right.Controls.Add(yaw);right.Controls.Add(detail);
  void ActionButton(string name,Action action){var b=new Button{Text=name,Width=300};b.Click+=(_,_)=>Run(action);right.Controls.Add(b);}
  ActionButton("Duplicar: elegir nuevo punto",()=>{if(list.SelectedIndex<0)throw new InvalidOperationException("Seleccione un encuentro");tool.SelectedIndex=1;status.Text="Clic en suelo válido para duplicar con la misma orientación. Se asignará otro ID.";});
  ActionButton("Eliminar seleccionado",()=>{int i=list.SelectedIndex;if(i<0)return;if(MessageBox.Show(this,"¿Eliminar "+data.encounters[i].id+" del borrador?","Confirmar",MessageBoxButtons.YesNo)==DialogResult.Yes){data.encounters.RemoveAt(i);Changed();}});
  ActionButton("Descartar cambios",()=>{if(!ConfirmDiscard())return;if(path!=null&&File.Exists(path))LoadFile(path);else{data=Document.Empty();dirty=false;RefreshList();}});
  right.Controls.Add(new Label{Width=305,Height=160,Text="Verde: suelo estático calculado; gameplay pendiente.\nRojo: altura/posición incompatible o no comprobada.\nRueda: zoom. Botón derecho: desplazar.\nGuardar no crea actores en la sesión actual.\nCierre completamente Dusklight antes de guardar y vuelva a iniciarlo."});
  Controls.Add(map);Controls.Add(right);Controls.Add(top);Controls.Add(status);
  list.SelectedIndexChanged+=(_,_)=>SelectDetails();
  enabled.CheckedChanged+=(_,_)=>EditMetadata();yaw.ValueChanged+=(_,_)=>EditMetadata();
  map.Paint+=Draw;map.MouseWheel+=Zoom;map.MouseDown+=Down;map.MouseMove+=Move;map.MouseUp+=Up;map.MouseEnter+=(_,_)=>map.Focus();
  FormClosing+=(_,e)=>{if(!ConfirmDiscard())e.Cancel=true;};
 }
 void Run(Action action){try{action();}catch(Exception e){status.Text="ERROR: "+e.Message;MessageBox.Show(this,e.Message,"DEVF: operación no completada",MessageBoxButtons.OK,MessageBoxIcon.Warning);}}
 bool ConfirmDiscard()=>!dirty||MessageBox.Show(this,"Hay cambios sin guardar. ¿Descartarlos?","DEVF",MessageBoxButtons.YesNo)==DialogResult.Yes;
 void New(){if(!ConfirmDiscard())return;data=Document.Empty();path=null;loadedHash=null;dirty=false;RefreshList();status.Text="Colección vacía. No hay activaciones por defecto.";}
 void Import(){using var d=new OpenFileDialog{Title="Importar archivo legítimo R03_00.arc de F_SP121",Filter="Archivo de sala|R03_00.arc|ARC|*.arc"};if(d.ShowDialog(this)!=DialogResult.OK)return;geometry=Geometry.Load(d.FileName);projection.Scale=Math.Min(map.Width/2200.0,map.Height/4200.0);map.Invalidate();status.Text="Mapa KCL auténtico cargado. Solo el rectángulo admitido; suelo estático no certifica eventos, ocupación o navegación.";}
 internal void AcceptImported(byte[] raw){geometry=Geometry.LoadBytes(raw);projection.CenterX=-1250;projection.CenterZ=61000;projection.Scale=Math.Min(map.Width/2200.0,map.Height/4200.0);map.Invalidate();status.Text="Geometría CP08 exacta desde disco; se conservan encuentros y guardado schema 1.";}
 void Open(){if(!ConfirmDiscard())return;using var d=new OpenFileDialog{Filter="Configuración JSON|*.json",InitialDirectory=Directory.Exists(dataDir)?dataDir:""};if(d.ShowDialog(this)==DialogResult.OK)LoadFile(d.FileName);}
 void LoadFile(string file){var b=Config.Read(file);data=Config.Decode(b);path=file;loadedHash=Config.Hash(b);dirty=false;RefreshList();status.Text="Configuración abierta. Importe el mapa para volver a comprobar alturas antes de guardar.";}
 void Restore(){if(!ConfirmDiscard())return;using var d=new OpenFileDialog{Title="Seleccionar respaldo para restaurar como borrador",Filter="Respaldo JSON|*.json",InitialDirectory=Directory.Exists(dataDir)?dataDir:""};if(d.ShowDialog(this)!=DialogResult.OK)return;data=Config.Decode(Config.Read(d.FileName));dirty=true;RefreshList();status.Text="Respaldo abierto como borrador; Guardar pedirá destino y conservará un respaldo del archivo actual.";}
 void Folder(){using var d=new FolderBrowserDialog{Description="Seleccione mod_data/dev.bokoblin.expansion de su instalación",UseDescriptionForTitle=true,SelectedPath=dataDir};if(d.ShowDialog(this)==DialogResult.OK){dataDir=d.SelectedPath;status.Text="Carpeta seleccionada. Guardar escribirá encounters.json allí, con confirmación.";} }
 void Save(){
  if(geometry==null&&data.encounters.Count>0)throw new InvalidOperationException("Importe la geometría antes de guardar ubicaciones.");
  if(geometry!=null)Config.GeometryCheck(data,geometry);Config.Validate(data);
  using var dialog=new SaveFileDialog{Title="Guardar para Bokoblin Expansion (juego cerrado)",Filter="JSON|*.json",FileName="encounters.json",InitialDirectory=Directory.Exists(dataDir)?dataDir:"",OverwritePrompt=true};
  if(dialog.ShowDialog(this)!=DialogResult.OK)return;string target=dialog.FileName;
  if(!string.Equals(Path.GetFileName(target),"encounters.json",StringComparison.OrdinalIgnoreCase)&&MessageBox.Show(this,"Este archivo será solo una copia. El mod lee encounters.json. ¿Guardar la copia?","DEVF",MessageBoxButtons.YesNo)!=DialogResult.Yes)return;
  if(MessageBox.Show(this,"Confirme que Dusklight está cerrado. Se guardará el JSON y se verificará un respaldo si ya existe. Los cambios se aplicarán al próximo arranque.","Guardar",MessageBoxButtons.OKCancel)!=DialogResult.OK)return;
  string? expected=string.Equals(target,path,StringComparison.OrdinalIgnoreCase)?loadedHash:File.Exists(target)?Config.Hash(Config.Read(target)):null;
  string backup=Config.Save(target,data,expected);path=target;loadedHash=Config.Hash(Config.Read(target));dirty=false;status.Text=backup.Length>0?"Guardado y respaldo verificados. Reinicie Dusklight.":"Guardado verificado. Reinicie Dusklight.";RefreshList();
 }
 void Launch(){if(dirty)throw new InvalidOperationException("Guarde o descarte los cambios antes de iniciar.");using var d=new OpenFileDialog{Title="Seleccionar Dusklight oficial 2.0.3",Filter="Dusklight|dusklight.exe|Ejecutable|*.exe"};if(d.ShowDialog(this)!=DialogResult.OK)return;if(MessageBox.Show(this,"Iniciar el ejecutable seleccionado con su configuración actual. Debe estar cerrado y usar solo el nuevo proveedor y consumidor. ¿Continuar?","Arranque frío",MessageBoxButtons.YesNo)!=DialogResult.Yes)return;Process.Start(new ProcessStartInfo(d.FileName){UseShellExecute=true,WorkingDirectory=Path.GetDirectoryName(d.FileName)});}
 void EditMetadata(){if(updating||list.SelectedIndex<0)return;int i=list.SelectedIndex;var old=data.encounters[i];var edited=old with{enabled=enabled.Checked,yaw_s16=(int)yaw.Value};data.encounters[i]=edited;try{Config.Validate(data);Changed(i);}catch(Exception e){data.encounters[i]=old;SelectDetails();status.Text=e.Message;}}
 void Changed(int selected=-1){dirty=true;RefreshList(selected);}
 void RefreshList(int selected=-1){updating=true;list.Items.Clear();foreach(var e in data.encounters)list.Items.Add(e.id+(e.enabled?" · activo":" · desactivado"));if(selected>=0&&selected<list.Items.Count)list.SelectedIndex=selected;updating=false;SelectDetails();map.Invalidate();Text="DEVF Map Editor CP09 0.9.0 — TEST privado"+(dirty?" *":"");}
 void SelectDetails(){updating=true;int i=list.SelectedIndex;enabled.Enabled=yaw.Enabled=i>=0;if(i>=0){var e=data.encounters[i];enabled.Checked=e.enabled;yaw.Value=e.yaw_s16;detail.Text=$"{e.id}\nF_SP121 / sala 3 / capa 6\nX {e.x:R}\nY {e.y:R}\nZ {e.z:R}\nAltura KCL + 5 unidades; no capturada de Link.";}else detail.Text="Seleccione un marcador.";updating=false;map.Invalidate();}
 string NewId(){for(int i=1;i<10000;i++){var s="bokoblin-"+i.ToString("000");if(!data.encounters.Any(e=>e.id==s))return s;}throw new InvalidOperationException("IDs agotados");}
 void Down(object? sender,MouseEventArgs e){last=e.Location;if(e.Button==MouseButtons.Right||(e.Button==MouseButtons.Left&&tool.SelectedIndex==2)){dragging=true;return;}if(e.Button!=MouseButtons.Left)return;
  var selected=data.encounters.Select((p,i)=>(p,i,screen:projection.Screen(p.x,p.z,map.Size))).Where(v=>Math.Abs(v.screen.X-e.X)<12&&Math.Abs(v.screen.Y-e.Y)<12).Select(v=>v.i).DefaultIfEmpty(-1).First();
  if(tool.SelectedIndex==0){list.SelectedIndex=selected;moving=selected;return;}
  Run(()=>{if(geometry==null)throw new InvalidOperationException("Importe R03_00.arc primero.");if(data.encounters.Count>=8)throw new InvalidOperationException("Máximo 8 encuentros.");var w=projection.World(e.Location,map.Size);float y=geometry.Place(w.X,w.Z);int angle=list.SelectedIndex>=0?data.encounters[list.SelectedIndex].yaw_s16:0;
   var n=new Encounter(NewId(),list.SelectedIndex<0||data.encounters[list.SelectedIndex].enabled,"E_OC2","F_SP121",3,6,w.X,y,w.Z,angle);var draft=data with{encounters=data.encounters.Append(n).ToList()};Config.Validate(draft);data=draft;Changed(data.encounters.Count-1);status.Text="Nuevo punto con apoyo KCL estático. Aparición, eventos y navegación requieren QA humana.";});
 }
 void Move(object? sender,MouseEventArgs e){if(dragging){projection.CenterX-=(e.X-last.X)/projection.Scale;projection.CenterZ+=(e.Y-last.Y)/projection.Scale;last=e.Location;map.Invalidate();return;}
  if(geometry==null)return;var w=projection.World(e.Location,map.Size);try{float y=geometry.Place(w.X,w.Z);status.Text=$"Apoyo estático X={w.X:F2} Y={y:F2} Z={w.Z:F2} · aparición/ocupación runtime pendiente";}catch(InvalidDataException ex){status.Text="NO COLOCABLE: "+ex.Message;}
 }
 void Up(object? sender,MouseEventArgs e){dragging=false;if(moving<0)return;int i=moving;moving=-1;Run(()=>{if(geometry==null)throw new InvalidOperationException("Importe el mapa.");var w=projection.World(e.Location,map.Size);float y=geometry.Place(w.X,w.Z);var draft=data with{encounters=data.encounters.ToList()};draft.encounters[i]=draft.encounters[i] with{x=w.X,y=y,z=w.Z};Config.Validate(draft);data=draft;Changed(i);});}
 void Zoom(object? sender,MouseEventArgs e){var before=projection.World(e.Location,map.Size);projection.Scale=Math.Clamp(projection.Scale*(e.Delta>0?1.2:1/1.2),.03,3);var after=projection.World(e.Location,map.Size);projection.CenterX+=before.X-after.X;projection.CenterZ+=before.Z-after.Z;map.Invalidate();}
 void Draw(object? sender,PaintEventArgs e){var g=e.Graphics;g.SmoothingMode=System.Drawing.Drawing2D.SmoothingMode.AntiAlias;
  if(geometry==null){g.DrawString("Importe R03_00.arc para mostrar el mapa real de colisión.",Font,Brushes.White,20,20);return;}
  foreach(var t in geometry.Triangles){var pts=new[]{projection.Screen(t.P.X,t.P.Z,map.Size),projection.Screen(t.Q.X,t.Q.Z,map.Size),projection.Screen(t.R.X,t.R.Z,map.Size)};using var brush=new SolidBrush(t.Floor?Color.FromArgb(80,60+(int)Math.Clamp((t.P.Y+7500)/30,0,110),100):Color.FromArgb(85,70,65));g.FillPolygon(brush,pts);g.DrawPolygon(Pens.DimGray,pts);}
  var a=projection.Screen(Geometry.MinX+100,Geometry.MaxZ-100,map.Size);var b=projection.Screen(Geometry.MaxX-100,Geometry.MinZ+100,map.Size);g.DrawRectangle(Pens.Gold,a.X,a.Y,b.X-a.X,b.Y-a.Y);
  for(int i=0;i<data.encounters.Count;i++){var p=data.encounters[i];var q=projection.Screen(p.x,p.z,map.Size);bool valid=false;try{valid=Math.Abs(geometry.Place((float)p.x,(float)p.z)-(float)p.y)<.01;}catch{}
   var color=!valid?Color.Red:!p.enabled?Color.Gray:Color.Lime;using var brush=new SolidBrush(color);g.FillEllipse(brush,q.X-7,q.Y-7,14,14);if(i==list.SelectedIndex)g.DrawEllipse(Pens.White,q.X-11,q.Y-11,22,22);double angle=p.yaw_s16*Math.PI/32768;g.DrawLine(Pens.White,q.X,q.Y,q.X+(float)Math.Sin(angle)*24,q.Y-(float)Math.Cos(angle)*24);g.DrawString(p.id,Font,Brushes.White,q.X+10,q.Y);}
  g.DrawString("N (+Z) ↑   X →    mapa técnico KCL / no mapa decorativo",Font,Brushes.White,10,10);
 }
 internal void ExerciseControlsForTest(Document source,Geometry g){
  data=Document.Empty();geometry=g;map.Size=new Size(900,650);tool.SelectedIndex=1;
  Point Pixel(Encounter e){var p=projection.Screen(e.x,e.z,map.Size);return Point.Round(p);}
  for(int i=0;i<2;i++){var p=Pixel(source.encounters[i]);Down(map,new MouseEventArgs(MouseButtons.Left,1,p.X,p.Y,0));if(data.encounters.Count!=i+1)throw new Exception("Actual add control failed");}
  list.SelectedIndex=0;enabled.Checked=false;yaw.Value=1234;
  if(data.encounters[0].enabled||data.encounters[0].yaw_s16!=1234)throw new Exception("Actual metadata controls failed");
  enabled.Checked=true;tool.SelectedIndex=0;var start=Pixel(data.encounters[0]);Down(map,new MouseEventArgs(MouseButtons.Left,1,start.X,start.Y,0));
  var dest=Pixel(source.encounters[2]);Up(map,new MouseEventArgs(MouseButtons.Left,1,dest.X,dest.Y,0));Config.GeometryCheck(data,g);
  if(Math.Abs(data.encounters[0].x-source.encounters[2].x)>10)throw new Exception("Actual drag control failed");
  using var bitmap=new Bitmap(map.Width,map.Height);map.DrawToBitmap(bitmap,map.ClientRectangle); // memory only, never distributed
 }
 internal void LoadForTest(Document d,Geometry g){data=d;geometry=g;RefreshList();}
}
