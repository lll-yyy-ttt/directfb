# DirectFB window manager and desktop

![image](https://github.com/lll-yyy-ttt/directfb/raw/master/my-linux-2018-05-08-13-02-31.png)

![image](https://github.com/lll-yyy-ttt/directfb/raw/master/my-linux-2018-05-08-13-22-12.png)

build:
export FUSION_INCLUDE=$(cd "$(dirname "../modules/usr/include/linux")"; pwd);

./configure --prefix=/usr/core/dfb-core -enable-sawman -disable-drmkms -enable-multi -enable-one --disable-x11 --with-gfxdrivers=none --with-inputdrivers=linuxinput --enable-fbdev --disable-sdl --disable-vnc --disable-osx --disable-video4linux --enable-zlib --enable-jpeg --enable-png --enable-gif --enable-freetype CPPFLAGS="-I$FUSION_INCLUDE

make && make install

new function:
IDirectFBWindow::SetName
IDirectFBWindow::GetName
IDirectFBWindow::SetHide
IDirectFBWindow::GetHide
IDirectFBWindow::ScreenToClient

ISaWManManager::Restack
ISaWManManager::SetHide
ISaWManManager::GetHide
ISaWManManager::SetMaximization
ISaWManManager::GetMaximization
ISaWManManager::RaiseToTop

SaWManCallbacks::WindowClose
SaWManCallbacks::WindowMin
SaWManCallbacks::WindowMax

发现的缺陷：
当一个用户的DFB C++对象（非共享的DFB对象）被释放后，再次用该DFB对象调用它的C++函数时，就会导致整个DFB系统的崩溃，这只是某个用户进程的错误，却导致了整个DFB系统的崩溃，这显然是不能接受的。解决该问题的方法是在每个进程中创建一个列表来保存已分配的C++对象，根据类型来分类这些C++对象，在每一个C++函数中都要检测this指针的有效性，如果该this指针在列表中找到，则这是一个有效的C++对象，否则抛出一个异常结束进程。

DFB核心概念

共享内存：
DFB中的绘图操作缓冲区都是从tmpfs文件系统中分配的文件，如果支持硬件加速的话，缓冲区可能是在显存中分配的，进程通过mmap函数将该文件映射到自己的地址空间来实现对缓冲区的读写操作。DFB中的所有共享数据都是以这样的方式创建的，在单核心进程的情况下，则是调用malloc函数分配的。

窗口合成：
当窗口的某一块区域需要进行重绘时，只需要调用sawman_update_window函数对该区域进行更新即可，包括窗口的显示与隐藏。DFB会对屏幕上该区域的窗口进行从下向上的合成操作。

结语：
本人不再维护和更新该代码，以后不再在linux下写代码了。本来打算移植webkit和mesa的directfb端，但实在提不起那样的精神来阅读和写代码了。很多人可能觉得linux是一个低效难以使用的操作系统，当你熟悉linux后就会发现其是一个高效、简单、安全的操作系统，你可以定制它的每一个操作，它只做你需要做的事情。在多媒体方面linux相对而言的确是不够强。

其它方案：
我个人觉得DFB在单核心进程的情况下支持得很好，但多进程核心支持得并不好，当前的DFB代码有很多BUG和缺陷极不稳定，X窗口系统的客户/服务器模型要比其稳定健壮得多，在个人看来DFB的性能并不比X窗口系统有多少的优势，在硬件加速方面X窗口系统也处理得很好，功能也强大，所以X窗口是最适合linux桌面系统。
我一直有个想法就是在内核空间实现窗口系统，窗口系统可以实现成linux的一个模块，以字符驱动模块的方式实现，每一个窗口可以看作一个虚拟的字符设备，创建一个窗口就是打开一个字符设备，关闭则相反，通过ioctl函数来对窗口进行各种操作。而绘图操作只需要在用户空间完成即可，就像使用DRM来绘图那样，绘图结束后通过ioctl进入系统空间进行flip操作，该操作就会执行窗口的合成，从而使窗口正确地显示到屏幕上。至于窗口的消息分发和窗口管理器（窗口大小的改变、移动、最大化、最小化、关闭）可以在一个用户进程中实现，内核模块只需要提供接口，实现窗口栈的合成使窗口能按正确的顺序显示，窗口的显示/隐藏和窗口管理相关的操作就可以了，用户可以通过ioctl函数来调用这些功能。

