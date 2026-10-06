param([Parameter(Mandatory=$true)][string]$Path)
$ErrorActionPreference='Stop'
$taskTarget=[IO.Path]::GetFullPath($Path)
if([IO.Path]::GetDirectoryName($taskTarget) -ne $PSScriptRoot -or -not (Test-Path -LiteralPath $taskTarget -PathType Leaf)){throw '只允许处理本目录已构建的设置程序'}
Add-Type -TypeDefinition @'
using System;using System.IO;using System.Text;using System.Runtime.InteropServices;using System.ComponentModel;
public static class GestureSettingsBranding {
 [DllImport("kernel32.dll",CharSet=CharSet.Unicode,SetLastError=true)]static extern IntPtr BeginUpdateResource(string file,bool delete);
 [DllImport("kernel32.dll",CharSet=CharSet.Unicode,SetLastError=true)]static extern bool UpdateResource(IntPtr update,IntPtr type,IntPtr name,ushort language,byte[] data,uint bytes);
 [DllImport("kernel32.dll",SetLastError=true)]static extern bool EndUpdateResource(IntPtr update,bool discard);
 static void Check(bool ok){if(!ok)throw new Win32Exception(Marshal.GetLastWin32Error());}
 static void Align(BinaryWriter writer){while(writer.BaseStream.Position%4!=0)writer.Write((byte)0);}
 static byte[] Block(string key,ushort type,ushort length,byte[] value,params byte[][] children){
  using(var stream=new MemoryStream())using(var writer=new BinaryWriter(stream)){
   writer.Write((ushort)0);writer.Write(length);writer.Write(type);writer.Write(Encoding.Unicode.GetBytes(key+"\0"));Align(writer);writer.Write(value);Align(writer);
   foreach(var child in children){writer.Write(child);Align(writer);}var result=stream.ToArray();var size=BitConverter.GetBytes(checked((ushort)result.Length));result[0]=size[0];result[1]=size[1];return result;
  }
 }
 static byte[] Text(string key,string value){return Block(key,1,(ushort)(value.Length+1),Encoding.Unicode.GetBytes(value+"\0"));}
 static byte[] Version(){
  var fixedInfo=new uint[]{0xFEEF04BD,0x10000,0x00000001,0x00010000,0x00000001,0x00010000,0x3f,0,0x40004,1,0,0,0};var bytes=new byte[52];Buffer.BlockCopy(fixedInfo,0,bytes,0,52);
  var table=Block("080404B0",1,0,new byte[0],Text("FileDescription","三指拖拽"),Text("ProductName","三指拖拽"),Text("InternalName","GestureSettings"),Text("OriginalFilename","GestureSettings.exe"),Text("FileVersion","0.1.1.0"),Text("ProductVersion","0.1.1.0"));
  var strings=Block("StringFileInfo",1,0,new byte[0],table);var translation=Block("Translation",0,4,new byte[]{4,8,0xB0,4});var variables=Block("VarFileInfo",1,0,new byte[0],translation);
  return Block("VS_VERSION_INFO",0,52,bytes,strings,variables);
 }
 public static void Apply(string file,string icon){
  var source=File.ReadAllBytes(icon);if(source.Length<6||BitConverter.ToUInt16(source,2)!=1)throw new InvalidDataException("Invalid existing icon");ushort count=BitConverter.ToUInt16(source,4);IntPtr update=BeginUpdateResource(file,false);Check(update!=IntPtr.Zero);bool saved=false;
  try{using(var stream=new MemoryStream())using(var writer=new BinaryWriter(stream)){
    writer.Write((ushort)0);writer.Write((ushort)1);writer.Write(count);
    for(int i=0;i<count;i++){int entry=6+i*16;uint size=BitConverter.ToUInt32(source,entry+8),offset=BitConverter.ToUInt32(source,entry+12);if(offset+size>source.Length)throw new InvalidDataException("Invalid icon frame");var data=new byte[size];Buffer.BlockCopy(source,(int)offset,data,0,(int)size);Check(UpdateResource(update,new IntPtr(3),new IntPtr(i+1),0,data,size));writer.Write(source,entry,12);writer.Write((ushort)(i+1));}
    var group=stream.ToArray();Check(UpdateResource(update,new IntPtr(14),new IntPtr(1),0,group,(uint)group.Length));
   }
   var version=Version();Check(UpdateResource(update,new IntPtr(16),new IntPtr(1),0x0804,version,(uint)version.Length));Check(EndUpdateResource(update,false));saved=true;
  }finally{if(!saved)EndUpdateResource(update,true);}
 }
}
'@ -ErrorAction Stop
[GestureSettingsBranding]::Apply($taskTarget,(Join-Path $PSScriptRoot 'Gesture.ico'))
$taskInfo=[Diagnostics.FileVersionInfo]::GetVersionInfo($taskTarget)
if($taskInfo.FileDescription -ne '三指拖拽' -or $taskInfo.ProductName -ne '三指拖拽'){throw '程序名称资源回读不一致'}
