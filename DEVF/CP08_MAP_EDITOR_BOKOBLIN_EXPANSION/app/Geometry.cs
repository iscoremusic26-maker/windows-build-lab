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
 static uint U(byte[] b,int p)=>BinaryPrimitives.ReadUInt32BigEndian(b.AsSpan(p,4));
 static ushort S(byte[] b,int p)=>BinaryPrimitives.ReadUInt16BigEndian(b.AsSpan(p,2));
 static float F(byte[] b,int p)=>BitConverter.Int32BitsToSingle(unchecked((int)U(b,p)));
 static V Vec(byte[] b,int p)=>new(F(b,p),F(b,p+4),F(b,p+8));
 static int I(uint u)=>checked((int)u);
 static void Check(bool b,string s){if(!b)throw new InvalidDataException(s);}
 static byte[] Yaz(byte[] b){
  if(!b.AsSpan(0,4).SequenceEqual("Yaz0"u8))return b;
  int size=I(U(b,4));Check(size>0&&size<100_000_000,"Tamaño Yaz0");byte[] o=new byte[size];int p=16,n=0;
  while(n<size){int code=b[p++];for(int bit=0;bit<8&&n<size;bit++){if((code&(128>>bit))!=0)o[n++]=b[p++];else{int a=b[p++],c=b[p++],d=((a&15)<<8)|c,len=a>>4;len=len==0?b[p++]+18:len+2;Check(d<n&&n+len<=size,"Yaz0 referencia");for(int j=0;j<len;j++){o[n]=o[n-d-1];n++;}}}}return o;
 }
 static Dictionary<string,byte[]> Rarc(byte[] raw){
  var b=Yaz(raw);Check(b.AsSpan(0,4).SequenceEqual("RARC"u8)&&U(b,4)==b.Length&&U(b,8)==32,"RARC cabecera");
  int dat=32+I(U(b,12)),nodes=I(U(b,32)),no=32+I(U(b,36)),files=I(U(b,40)),fo=32+I(U(b,44)),ss=I(U(b,48)),so=32+I(U(b,52));
  Check(nodes>0&&nodes<10000&&files<100000&&so+ss<=b.Length,"RARC límites");
  var output=new Dictionary<string,byte[]>();var stack=new HashSet<int>();
  string Name(int off){Check(off>=0&&off<ss,"Nombre");int end=Array.IndexOf(b,(byte)0,so+off,ss-off);Check(end>=0,"Nombre sin terminador");var s=Encoding.ASCII.GetString(b,so+off,end-so-off);Check(!s.Contains('/')&&!s.Contains('\\'),"Nombre inseguro");return s;}
  void Walk(int node,string prefix){Check(node>=0&&node<nodes&&stack.Add(node),"RARC ciclo");int at=no+node*16,count=S(b,at+10),first=I(U(b,at+12));Check(first+count<=files,"RARC entradas");
   for(int i=first;i<first+count;i++){int e=fo+i*20;uint bits=U(b,e+4);string name=Name((int)(bits&0xffffff));if(name=="."||name=="..")continue;int off=I(U(b,e+8)),size=I(U(b,e+12));if(((bits>>24)&2)!=0)Walk(off,prefix+name+"/");else{Check(size>=0&&dat+off>=0&&(long)dat+off+size<=b.Length,"RARC fichero");output.Add(prefix+name,b.AsSpan(dat+off,size).ToArray());}}stack.Remove(node);
  }Walk(0,"");return output;
 }
 public static Geometry Load(string path) {
  var fi=new FileInfo(path);Check(fi.Length>=32&&fi.Length<=100_000_000,"ARC tamaño");
  var raw=File.ReadAllBytes(path);Check(Convert.ToHexString(SHA256.HashData(raw)).ToLowerInvariant()==Hash,"R03_00.arc no coincide con la revisión admitida. No se importó geometría.");
  var arc=Rarc(raw);
  byte[] Find(string suffix)=>Yaz(arc.Single(p=>p.Key.EndsWith(suffix,StringComparison.Ordinal)).Value);
  var b=Find("kcl/room.kcl");var plc=Find("plc/room.plc");
  int op=I(U(b,0)),on=I(U(b,4)),ot=I(U(b,8)),ob=I(U(b,12));
  Check(56<=op&&op<on&&on<ot&&ot<ob&&ob<b.Length&&(on-op)%12==0&&(ob-ot)%16==0,"KCL offsets");
  Check(plc.AsSpan(0,4).SequenceEqual("SPLC"u8)&&S(plc,4)==20,"PLC");int count=S(plc,6);
  V Norm(int index){int at=on+12*index;Check(at>=on&&at+12<=ot+16,"KCL normal");return Vec(b,at);}
  var g=new Geometry();
  for(int i=1;i<(ob-ot)/16;i++){int at=ot+16*i;double h=F(b,at);int pi=S(b,at+4),attr=S(b,at+14);Check(pi<(on-op)/12&&attr<count&&double.IsFinite(h),"KCL prisma");
   V p=Vec(b,op+12*pi),n=Norm(S(b,at+6)),a=Norm(S(b,at+8)),c1=Norm(S(b,at+10)),c2=Norm(S(b,at+12));
   var r1=n.Cross(a);var r2=c1.Cross(n);Check(Math.Abs(r1.Dot(c2))>1e-8&&Math.Abs(r2.Dot(c2))>1e-8,"KCL degenerado");
   var q=p+r2*(h/r2.Dot(c2));var r=p+r1*(h/r1.Dot(c2));Check(p.Finite&&q.Finite&&r.Finite,"KCL no finito");
   if(Math.Max(p.X,Math.Max(q.X,r.X))<MinX-250||Math.Min(p.X,Math.Min(q.X,r.X))>MaxX+250||Math.Max(p.Z,Math.Max(q.Z,r.Z))<MinZ-250||Math.Min(p.Z,Math.Min(q.Z,r.Z))>MaxZ+250)continue;
   var t=new Triangle(p,q,r,n,a,c1,c2,h,U(plc,8+attr*20),U(plc,12+attr*20),U(plc,24+attr*20));g.Triangles.Add(t);
   for(int x=Cell(Math.Max(MinX-250,Math.Min(p.X,Math.Min(q.X,r.X))));x<=Cell(Math.Min(MaxX+250,Math.Max(p.X,Math.Max(q.X,r.X))));x++)
   for(int z=Cell(Math.Max(MinZ-250,Math.Min(p.Z,Math.Min(q.Z,r.Z))));z<=Cell(Math.Min(MaxZ+250,Math.Max(p.Z,Math.Max(q.Z,r.Z))));z++){if(!g.cells.TryGetValue((x,z),out var list))g.cells[(x,z)]=list=new();list.Add(t);}
  }Check(g.Triangles.Count>0,"Región vacía");return g;
 }
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
