# gimxserial

The gimxserial library is a serial access library supporting asynchronous IO.  
It has a compilation dependency on gimxpoll headers, and on gimxcommon source code.  

Compilation:

```
git clone https://github.com/matlo/gimxpoll.git
git clone https://github.com/matlo/gimxcommon.git
git clone https://github.com/matlo/gimxlog.git
CPPFLAGS="-I../" make -C gimxlog
git clone https://github.com/matlo/gimxtime.git
CPPFLAGS="-I../" make -C gimxtime
git clone https://github.com/matlo/gimxserial.git
CPPFLAGS="-I../" make -C gimxserial
```
