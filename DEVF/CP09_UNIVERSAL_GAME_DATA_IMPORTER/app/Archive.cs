using System.Text;
using System.Buffers.Binary;
namespace DevfMap;
public static class Archive {
 public const int Max=96*1024*1024;
 static void C(bool b,string s)=>Disc.Require(b,s);
 static int I(uint v)=>checked((int)v);
 public static ushort S(byte[] b,int p)=>BinaryPrimitives.ReadUInt16BigEndian(b.AsSpan(p,2));
 public static byte[] Yaz(byte[] b,CancellationToken token=default){
  C(b.Length>=4&&b.Length<=Max,"ARCHIVE_SIZE");if(!b.AsSpan(0,4).SequenceEqual("Yaz0"u8))return b;
  C(b.Length>=16,"YAZ_HEADER");int size=I(Disc.BE(b,4));C(size>0&&size<=Max,"YAZ_EXPANSION_LIMIT");var output=new byte[size];int p=16,n=0;
  int Next(){C(p<b.Length,"YAZ_TRUNCATED");return b[p++];}
  while(n<size){token.ThrowIfCancellationRequested();int code=Next();for(int bit=0;bit<8&&n<size;bit++){
   if((code&(128>>bit))!=0)output[n++]=(byte)Next();else{int a=Next(),c=Next(),distance=((a&15)<<8)|c,len=a>>4;len=len==0?Next()+18:len+2;C(distance<n&&len<=size-n,"YAZ_REFERENCE");for(int j=0;j<len;j++){output[n]=output[n-distance-1];n++;}}
  }}return output;
 }
 public static Dictionary<string,ReadOnlyMemory<byte>> Read(byte[] raw,CancellationToken token=default){
  byte[] b=Yaz(raw,token);C(b.Length>=64&&b.AsSpan(0,4).SequenceEqual("RARC"u8)&&Disc.BE(b,4)==b.Length&&Disc.BE(b,8)==32,"RARC_HEADER");
  int dat=checked(32+I(Disc.BE(b,12))),nodes=I(Disc.BE(b,32)),no=checked(32+I(Disc.BE(b,36))),files=I(Disc.BE(b,40)),fo=checked(32+I(Disc.BE(b,44))),ss=I(Disc.BE(b,48)),so=checked(32+I(Disc.BE(b,52)));
  bool Range(int at,long size)=>at>=0&&size>=0&&(long)at+size<=b.Length;
  C(nodes>0&&nodes<=10000&&files<=100000&&Range(no,(long)nodes*16)&&Range(fo,(long)files*20)&&Range(so,ss)&&dat<=b.Length,"RARC_TABLE_RANGE");
  var result=new Dictionary<string,ReadOnlyMemory<byte>>(StringComparer.OrdinalIgnoreCase);var stack=new HashSet<int>();
  string Name(int off){C(off>=0&&off<ss,"RARC_NAME_OFFSET");int end=Array.IndexOf(b,(byte)0,so+off,ss-off);C(end>=0&&end-so-off<=128,"RARC_NAME_END");string s=Encoding.ASCII.GetString(b,so+off,end-so-off);C(s is "." or ".."||Disc.SafeName(s),"RARC_NAME_UNSAFE");return s;}
  void Walk(int node,string prefix,int depth){
   token.ThrowIfCancellationRequested();C(node>=0&&node<nodes&&depth<=32&&stack.Add(node),"RARC_CYCLE_DEPTH");int at=no+node*16,count=S(b,at+10),first=I(Disc.BE(b,at+12));C((long)first+count<=files,"RARC_ENTRY_RANGE");
   for(int i=first;i<first+count;i++){token.ThrowIfCancellationRequested();int e=fo+i*20;uint bits=Disc.BE(b,e+4);string name=Name((int)(bits&0xffffff));if(name is "." or "..")continue;string path=prefix+name;C(path.Length<=512,"RARC_PATH_LENGTH");int off=I(Disc.BE(b,e+8)),size=I(Disc.BE(b,e+12));if(((bits>>24)&2)!=0)Walk(off,path+"/",depth+1);else{long start=(long)dat+off;C(start<=int.MaxValue&&Range((int)start,size),"RARC_DATA_RANGE");C(result.TryAdd(path,b.AsMemory((int)start,size)),"RARC_DUPLICATE");}}
   // A directory node may be visited only once: reject aliased graphs as well as cycles.
  }Walk(0,"",0);return result;
 }
 public static string[] LayerChunks(Dictionary<string,ReadOnlyMemory<byte>> members){
  var found=new SortedSet<string>();
  foreach(var e in members.Where(e=>e.Key.EndsWith(".dzr",StringComparison.OrdinalIgnoreCase)||e.Key.EndsWith(".dzs",StringComparison.OrdinalIgnoreCase))){
   var b=Yaz(e.Value.ToArray());C(b.Length>=4,"DZX_HEADER");uint n=Disc.BE(b,0);C(n<=1024&&4L+n*12<=b.Length,"DZX_TABLE");
   for(int i=0;i<n;i++){int at=4+i*12;string tag=Encoding.ASCII.GetString(b,at,4);C(Disc.BE(b,at+8)<=b.Length,"DZX_OFFSET");if(tag.Length==4&&"0123456789abcdefABCDEF".Contains(tag[3])&&(tag.StartsWith("ACT")||tag.StartsWith("SCO")||tag.StartsWith("TRE")||tag.StartsWith("Doo")))found.Add(tag);}
  }return found.ToArray();
 }
}
public sealed record Mesh(List<Triangle> Triangles,double MinX,double MaxX,double MinY,double MaxY,double MinZ,double MaxZ,int[] PlcRooms,string KclPath,string PlcPath,string KclHash,string PlcHash){
 public static bool Floor(Triangle t,int room)=>t.N.Y>=.7&&(t.C0&(1u<<14))==0&&(t.C0&(1u<<16))==0&&(t.C4&(1u<<8))==0&&((t.C4>>20)&255)==room&&((t.C1>>19)&31)==0&&((t.C0>>24)&15)==0&&(t.C0&63)==63;
 public double Ground(double x,double z,int room){
  var hits=Triangles.Where(t=>t.N.Y>.1).Select(t=>(t,y:t.Hit(x,z))).Where(h=>h.y.HasValue&&double.IsFinite(h.y.Value)).OrderByDescending(h=>h.y).ToArray();
  Disc.Require(hits.Length>0&&Floor(hits[0].t,room),"FLOOR_UNVERIFIED");double y=hits[0].y!.Value;Disc.Require(!hits.Any(h=>Math.Abs(h.y!.Value-y)>30),"FLOOR_AMBIGUOUS");return y;
 }
}
public static class GeometryReader {
 public static Mesh Read(Dictionary<string,ReadOnlyMemory<byte>> arc,CancellationToken token=default){
  var pairs=arc.Keys.Where(k=>k.EndsWith(".kcl",StringComparison.OrdinalIgnoreCase)).ToArray();Disc.Require(pairs.Length==1,"GEOMETRY_KCL_ABSENT_OR_MULTIPLE");
  var plcs=arc.Keys.Where(k=>k.EndsWith(".plc",StringComparison.OrdinalIgnoreCase)).ToArray();Disc.Require(plcs.Length==1,"GEOMETRY_PLC_ABSENT_OR_MULTIPLE");
  var b=Archive.Yaz(arc[pairs[0]].ToArray(),token);var plc=Archive.Yaz(arc[plcs[0]].ToArray(),token);Disc.Require(b.Length>=56&&plc.Length>=8,"KCL_PLC_HEADER");
  int op=checked((int)Disc.BE(b,0)),on=checked((int)Disc.BE(b,4)),ot=checked((int)Disc.BE(b,8)),ob=checked((int)Disc.BE(b,12));
  Disc.Require(56<=op&&op<on&&on<ot&&ot<ob&&ob<b.Length&&(on-op)%12==0&&(ob-ot)%16==0&&(ob-ot)/16<=200001,"KCL_OFFSETS");
  Disc.Require(plc.AsSpan(0,4).SequenceEqual("SPLC"u8)&&Archive.S(plc,4)==20,"PLC_HEADER");int count=Archive.S(plc,6);Disc.Require(8L+count*20<=plc.Length,"PLC_TABLE");
  double F(int p)=>BitConverter.Int32BitsToSingle(unchecked((int)Disc.BE(b,p)));
  V Vec(int p){var v=new V(F(p),F(p+4),F(p+8));Disc.Require(v.Finite&&Math.Abs(v.X)<=1e7&&Math.Abs(v.Y)<=1e7&&Math.Abs(v.Z)<=1e7,"KCL_VECTOR");return v;}
  V Norm(int index){int at=on+12*index;Disc.Require(at>=on&&(long)at+12<=ot+16,"KCL_NORMAL");return Vec(at);}
  var list=new List<Triangle>();var rooms=new SortedSet<int>();double minx=double.MaxValue,miny=minx,minz=minx,maxx=double.MinValue,maxy=maxx,maxz=maxx;
  for(int i=1;i<(ob-ot)/16;i++){
   token.ThrowIfCancellationRequested();int at=ot+16*i;double h=F(at);int pi=Archive.S(b,at+4),attr=Archive.S(b,at+14);Disc.Require(pi<(on-op)/12&&attr<count&&double.IsFinite(h)&&h>0,"KCL_PRISM");
   V p=Vec(op+12*pi),n=Norm(Archive.S(b,at+6)),a=Norm(Archive.S(b,at+8)),b1=Norm(Archive.S(b,at+10)),c=Norm(Archive.S(b,at+12));var r1=n.Cross(a);var r2=b1.Cross(n);
   Disc.Require(Math.Abs(r1.Dot(c))>1e-8&&Math.Abs(r2.Dot(c))>1e-8,"KCL_DEGENERATE");
   var q=p+r2*(h/r2.Dot(c));var r=p+r1*(h/r1.Dot(c));
   foreach(var v in new[]{p,q,r}){Disc.Require(v.Finite&&Math.Abs(v.X)<=1e7&&Math.Abs(v.Y)<=1e7&&Math.Abs(v.Z)<=1e7,"KCL_VERTEX");minx=Math.Min(minx,v.X);maxx=Math.Max(maxx,v.X);miny=Math.Min(miny,v.Y);maxy=Math.Max(maxy,v.Y);minz=Math.Min(minz,v.Z);maxz=Math.Max(maxz,v.Z);}
   uint c0=Disc.BE(plc,8+attr*20),c1=Disc.BE(plc,12+attr*20),c4=Disc.BE(plc,24+attr*20);rooms.Add((int)((c4>>20)&255));list.Add(new(p,q,r,n,a,b1,c,h,c0,c1,c4));
  }Disc.Require(list.Count>0,"GEOMETRY_EMPTY");return new(list,minx,maxx,miny,maxy,minz,maxz,rooms.ToArray(),pairs[0],plcs[0],Disc.Hash(b),Disc.Hash(plc));
 }
}
