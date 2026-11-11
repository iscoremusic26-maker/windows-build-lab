using System.Buffers.Binary;
using System.Security.Cryptography;
using System.Text;
namespace DevfMap;
public sealed record DiscEntry(string Path,long Offset,int Length);
public sealed class Disc:IDisposable {
 readonly FileStream file; readonly long[]? blocks; readonly int blockSize; readonly long readablePhysicalLength; public long UnmappedTailBytes{get;} public long Length{get;} public string Format{get;} public string Id{get;} public int Revision{get;} public string FstHash{get;} public List<DiscEntry> Files{get;}=new();
 public static void Require(bool b,string code){if(!b)throw new InvalidDataException(code);}
 public static uint BE(byte[] b,int p)=>BinaryPrimitives.ReadUInt32BigEndian(b.AsSpan(p,4));
 public static string Hash(byte[] b)=>Convert.ToHexString(SHA256.HashData(b)).ToLowerInvariant();
 public static bool SafeName(string s)=>s.Length is >0 and <=128 && s!="."&&s!=".."&&s.All(c=>c>=32&&c<127&&!"\\/:*?\"<>|".Contains(c));
 // Importer policy, not a CISO padding specification. This suffix is never mapped into logical disc reads.
 internal const int MaxUnmappedTailBytes=4096;
 internal static long ValidateCisoPhysicalLength(long mappedEnd,long actualLength){
  Require(mappedEnd>=32768&&mappedEnd<=32768L+32760L*32*1024*1024,"CISO_PHYSICAL_LENGTH");
  Require(actualLength>=mappedEnd,"CISO_PHYSICAL_LENGTH"); // Every present block must be complete.
  long tail=actualLength-mappedEnd;
  Require(tail<=MaxUnmappedTailBytes,"CISO_UNMAPPED_TAIL_LIMIT");
  return tail;
 }
 public Disc(string path){
  file=new FileStream(path,FileMode.Open,FileAccess.Read,FileShare.Read,65536,FileOptions.RandomAccess);
  try{
   Require(file.Length>=0x440,"DISC_TRUNCATED");var h=new byte[8];file.ReadExactly(h);
   if(h.AsSpan(0,4).SequenceEqual("CISO"u8)){
    Format="CISO sparse 0x8000";blockSize=checked((int)BinaryPrimitives.ReadUInt32LittleEndian(h.AsSpan(4)));
    Require(blockSize>=32768&&blockSize<=32*1024*1024&&(blockSize&(blockSize-1))==0,"CISO_BLOCK_SIZE");
    Require(file.Length>=32768,"CISO_HEADER");var map=new byte[32760];file.ReadExactly(map);blocks=new long[map.Length];long physical=32768;
    for(int i=0;i<map.Length;i++){Require(map[i]<=1,"CISO_MAP_VALUE");blocks[i]=map[i]==1?physical:-1;if(map[i]==1)physical=checked(physical+blockSize);}
    UnmappedTailBytes=ValidateCisoPhysicalLength(physical,file.Length);readablePhysicalLength=physical;Length=(long)map.Length*blockSize;
   }else{Format="GameCube ISO/GCM";Length=readablePhysicalLength=file.Length;}
   var header=Read(0,0x440);Require(BE(header,0x1c)==0xc2339f3d,"NOT_GAMECUBE_IMAGE");
   Id=Encoding.ASCII.GetString(header,0,6);Revision=header[7];Require(Id=="GZ2E01"&&header[6]==0&&Revision==0,"UNSUPPORTED_DISC_ID_OR_REVISION");
   long off=BE(header,0x424);int size=checked((int)BE(header,0x428));Require(size>=12&&size<=16*1024*1024,"FST_SIZE");
   var fst=Read(off,size);FstHash=Hash(fst);ParseFst(fst);
  }catch{file.Dispose();throw;}
 }
 public byte[] Read(long offset,int count){
  Require(offset>=0&&count>=0&&count<=96*1024*1024&&offset<=Length-count,"DISC_READ_RANGE");
  var result=new byte[count];int done=0;
  while(done<count){long at=offset+done;int take=count-done;long physical=at;
   if(blocks!=null){int index=checked((int)(at/blockSize)),within=(int)(at%blockSize);take=Math.Min(take,blockSize-within);physical=blocks[index]<0?-1:blocks[index]+within;}
   if(physical>=0){Require(physical<=readablePhysicalLength-take,"DISC_PHYSICAL_READ_RANGE");file.Position=physical;file.ReadExactly(result.AsSpan(done,take));}done+=take;
  }return result;
 }
 void ParseFst(byte[] b){
  Require((BE(b,0)>>24)==1&&BE(b,4)==0,"FST_ROOT");int count=checked((int)BE(b,8));Require(count>=1&&count<=100000&&(long)count*12<b.Length,"FST_COUNT");int strings=count*12;
  string Name(int offset){Require(offset>=0&&offset<b.Length-strings,"FST_NAME_OFFSET");int start=strings+offset,end=Array.IndexOf(b,(byte)0,start);Require(end>=start&&end-start<=128,"FST_NAME_END");var s=Encoding.ASCII.GetString(b,start,end-start);Require(SafeName(s)&&b.AsSpan(start,end-start).ToArray().All(v=>v<128),"FST_UNSAFE_NAME");return s;}
  var paths=new HashSet<string>(StringComparer.OrdinalIgnoreCase);
  int Walk(int index,int end,string prefix,int depth){
   Require(depth<=32,"FST_DEPTH");int i=index+1;
   while(i<end){uint word=BE(b,i*12);int type=(int)(word>>24);Require(type is 0 or 1,"FST_ENTRY_TYPE");string path=prefix+Name((int)(word&0xffffff));Require(path.Length<=512&&paths.Add(path),"FST_DUPLICATE_PATH");
    uint a=BE(b,i*12+4),c=BE(b,i*12+8);
    if(type==1){Require(a==index&&c>i&&c<=end,"FST_DIRECTORY_RANGE");i=Walk(i,(int)c,path+"/",depth+1);}
    else{Require((long)a+c<=Length,"FST_FILE_RANGE");Require(c<=int.MaxValue,"FST_FILE_SIZE");Files.Add(new(path,a,(int)c));i++;}
   }Require(i==end,"FST_DIRECTORY_END");return i;
  }Walk(0,count,"",0);
 }
 public static string ImageHash(string path,CancellationToken cancel,IProgress<string>? progress=null){
  using var s=new FileStream(path,FileMode.Open,FileAccess.Read,FileShare.Read,1024*1024,FileOptions.SequentialScan);
  using var hash=IncrementalHash.CreateHash(HashAlgorithmName.SHA256);var buffer=new byte[1024*1024];long done=0;int last=-1,n;
  while((n=s.Read(buffer))>0){cancel.ThrowIfCancellationRequested();hash.AppendData(buffer,0,n);done+=n;int pct=(int)(100*done/s.Length);if(pct!=last){last=pct;progress?.Report("Identidad de imagen: "+pct+"%");}}
  return Convert.ToHexString(hash.GetHashAndReset()).ToLowerInvariant();
 }
 public void Dispose()=>file.Dispose();
}
