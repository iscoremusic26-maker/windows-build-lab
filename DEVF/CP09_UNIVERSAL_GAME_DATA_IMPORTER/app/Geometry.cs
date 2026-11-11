using System.Buffers.Binary;
using System.Security.Cryptography;
using System.Text;
namespace DevfMap;
public readonly record struct V(double X,double Y,double Z) {
 public static V operator +(V a,V b)=>new(a.X+b.X,a.Y+b.Y,a.Z+b.Z);
 public static V operator -(V a,V b)=>new(a.X-b.X,a.Y-b.Y,a.Z-b.Z);
 public static V operator *(V a,double b)=>new(a.X*b,a.Y*b,a.Z*b);
 public double Dot(V b)=>X*b.X+Y*b.Y+Z*b.Z;
 public V Cross(V b)=>new(Y*b.Z-Z*b.Y,Z*b.X-X*b.Z,X*b.Y-Y*b.X);
 public bool Finite=>double.IsFinite(X)&&double.IsFinite(Y)&&double.IsFinite(Z);
}
public sealed record Triangle(V P,V Q,V R,V N,V A,V B,V C,double Height,uint C0,uint C1,uint C4) {
 public bool Floor=>N.Y>=.7&&(C0&(1u<<14))==0&&(C0&(1u<<16))==0&&(C4&(1u<<8))==0&&((C4>>20)&255)==3&&((C1>>19)&31)==0&&((C0>>24)&15)==0&&(C0&63)==63;
 public double? Hit(double x,double z) {
  if(Math.Abs(N.Y)<.00001)return null;
  var d=new V(x-P.X,-((x-P.X)*N.X+(z-P.Z)*N.Z)/N.Y,z-P.Z);
  return d.Dot(A)<=.0075&&d.Dot(B)<=.0075&&d.Dot(C)<=Height+.0075&&d.Dot(C)>=-.0075?P.Y+d.Y:null;
 }
}
public sealed class Geometry {
 public const string Hash="dc53106667811c4cde18b35edb5279357f00ebecaef2f9de546aea9a5a2b06af";
 public const double MinX=-2250,MaxX=-250,MinZ=59000,MaxZ=63000;
 public List<Triangle> Triangles{get;}=new();
 readonly Dictionary<(int,int),List<Triangle>> cells=new();
 static int Cell(double v)=>(int)Math.Floor(v/250);
 public static Geometry Load(string path){var info=new FileInfo(path);Disc.Require(info.Length>=32&&info.Length<=Archive.Max,"ARC_SIZE");return LoadBytes(File.ReadAllBytes(path));}
 public static Geometry LoadBytes(byte[] raw){
  Disc.Require(Disc.Hash(raw)==Hash,"R03_00.arc no coincide con la revisión CP08 admitida.");
  var mesh=GeometryReader.Read(Archive.Read(raw));var g=new Geometry();
  foreach(var t in mesh.Triangles){var p=t.P;var q=t.Q;var r=t.R;
   if(Math.Max(p.X,Math.Max(q.X,r.X))<MinX-250||Math.Min(p.X,Math.Min(q.X,r.X))>MaxX+250||Math.Max(p.Z,Math.Max(q.Z,r.Z))<MinZ-250||Math.Min(p.Z,Math.Min(q.Z,r.Z))>MaxZ+250)continue;
   g.Triangles.Add(t);
   for(int x=Cell(Math.Max(MinX-250,Math.Min(p.X,Math.Min(q.X,r.X))));x<=Cell(Math.Min(MaxX+250,Math.Max(p.X,Math.Max(q.X,r.X))));x++)
   for(int z=Cell(Math.Max(MinZ-250,Math.Min(p.Z,Math.Min(q.Z,r.Z))));z<=Cell(Math.Min(MaxZ+250,Math.Max(p.Z,Math.Max(q.Z,r.Z))));z++){if(!g.cells.TryGetValue((x,z),out var list))g.cells[(x,z)]=list=new();list.Add(t);}
  }Disc.Require(g.Triangles.Count>0,"Región vacía");return g;
 }
 static void Check(bool b,string s)=>Disc.Require(b,s);
 IEnumerable<Triangle> At(double x,double z)=>cells.TryGetValue((Cell(x),Cell(z)),out var a)?a:Enumerable.Empty<Triangle>();
 public double Ground(double x,double z){
  var hits=At(x,z).Where(t=>t.N.Y>.1).Select(t=>(t,y:t.Hit(x,z))).Where(v=>v.y is >-7505 and <-5005).OrderByDescending(v=>v.y).ToArray();
  if(hits.Length==0||!hits[0].t.Floor)throw new InvalidDataException("Suelo ausente o PLC/pendiente incompatible");
  double y=hits[0].y!.Value;
  if(hits.Any(v=>Math.Abs(v.y!.Value-y)>30))throw new InvalidDataException("Capas de suelo superpuestas: altura ambigua");
  return y;
 }
 public float Place(float x,float z) {
  Check(float.IsFinite(x)&&float.IsFinite(z)&&x>=MinX+100&&x<=MaxX-100&&z>=MinZ+100&&z<=MaxZ-100,"Fuera del área con margen");
  double y=Ground(x,z),lo=y,hi=y;
  for(int dx=-100;dx<=100;dx+=50)for(int dz=-100;dz<=100;dz+=50){double h=Ground(x+dx,z+dz);lo=Math.Min(lo,h);hi=Math.Max(hi,h);}
  Check(hi-lo<=50,"Desnivel de huella superior a 50");
  // Static obstacle screen only; dynamic actors, events and navigation remain human QA.
  for(int dx=-200;dx<=200;dx+=200)for(int dz=-200;dz<=200;dz+=200)
   foreach(var t in At(x+dx,z+dz).Where(t=>t.N.Y<.5&&(t.C0&(1u<<14))==0)){
    double miny=Math.Min(t.P.Y,Math.Min(t.Q.Y,t.R.Y)),maxy=Math.Max(t.P.Y,Math.Max(t.Q.Y,t.R.Y));if(maxy<y+5||miny>y+250)continue;
    double Dist(V a,V b){double vx=b.X-a.X,vz=b.Z-a.Z,d=vx*vx+vz*vz;double u=d==0?0:Math.Clamp(((x-a.X)*vx+(z-a.Z)*vz)/d,0,1);return Math.Sqrt(Math.Pow(x-a.X-u*vx,2)+Math.Pow(z-a.Z-u*vz,2));}
    Check(Math.Min(Dist(t.P,t.Q),Math.Min(Dist(t.Q,t.R),Dist(t.R,t.P)))>=200,"Pared estática a menos de 200");
   }
  return (float)(y+5); // Explicit native spawn clearance, not a replacement for captured height.
 }
}
public sealed class Projection {
 public double CenterX=-1250,CenterZ=61000,Scale=.15;
 public PointF Screen(double x,double z,Size s)=>new((float)(s.Width/2.0+(x-CenterX)*Scale),(float)(s.Height/2.0-(z-CenterZ)*Scale));
 public (float X,float Z) World(PointF p,Size s)=>((float)(CenterX+(p.X-s.Width/2.0)/Scale),(float)(CenterZ-(p.Y-s.Height/2.0)/Scale));
}
