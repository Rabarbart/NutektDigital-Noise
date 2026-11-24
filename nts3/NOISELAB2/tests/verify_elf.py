#!/usr/bin/env python3
import hashlib, json, pathlib, struct, sys
p=pathlib.Path(sys.argv[1]);b=p.read_bytes()
errors=[]
def check(ok,msg):
    if not ok:errors.append(msg)
check(b[:4]==b"\x7fELF","ELF magic")
if len(b)<52 or b[4]!=1 or b[5]!=1:raise SystemExit("not ELF32 little endian")
e=struct.unpack_from('<16sHHIIIIIHHHHHH',b)
et,em,flags,shoff,shnum,shentsize,shstrndx=e[1],e[2],e[7],e[6],e[12],e[11],e[13]
check(et==3 and em==40,"ARM ET_DYN")
check(flags>>24==5 and flags&0x400,"EABI5 + hardfloat")
ss=[struct.unpack_from('<IIIIIIIIII',b,shoff+i*shentsize) for i in range(shnum)]
strsec=ss[shstrndx];strings=b[strsec[4]:strsec[4]+strsec[5]]
sec={}
for s in ss:
    name=strings[s[0]:].split(b'\0',1)[0].decode('ascii','ignore')
    sec[name]=s
check('.unit_header' in sec,"missing .unit_header")
if '.unit_header' in sec:
    s=sec['.unit_header'];check(s[5]==376, f"unit_header section size {s[5]} != 376")
    if s[5]>=376:
        h=b[s[4]:s[4]+376]
        vals=struct.unpack_from('<6I20s3I',h)
        size,target,api,dev,uid,vers,name,res0,res1,count=vals
        check(size==376,"header_size")
        check(target==0x0607,"target 0x0607")
        check(api==0x00020000,"API version")
        check(dev==0x42415254 and uid==0x4E4C3230,"prototype developer/unit ID")
        check(count==8,"eight remappable params")
        names=["TYPE","ATTACK","DECAY","RELEASE","TONE","RESONANCE","TEXTURE","LEVEL"]
        for i,expected in enumerate(names):
            lo,hi,cent,init,typ,frac,n=struct.unpack_from('<hhhhBB22s',h,56+i*32)
            text=n.split(b'\0')[0].decode('ascii','replace')
            check(text==expected, f"param {i} name {text}")
            check(lo<=cent<=hi and lo<=init<=hi, f"param {i} bounds")
            assign,curve,mi,ma,default=struct.unpack_from('<BBhhh',h,312+i*8)
            check(0<=assign<=3 and 0<=mi<=ma<=hi and mi<=default<=ma,f"mapping {i}")
            expect={2:2,4:1,7:3}.get(i,0)
            check(assign==expect, f"mapping assignment {i}")
details={"path":str(p),"sha256":hashlib.sha256(b).hexdigest(),"size":len(b),"errors":errors}
print(json.dumps(details,indent=2))
sys.exit(1 if errors else 0)
