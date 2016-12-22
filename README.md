# FFmpeg Examples

Trying to learn FFmpeg was almost impossible for me when I was attempting use it. The examples were all over the internet, but I had spent so much time wrestling with outdated codes: deprecated functions and really, really sparse documentation. I don't think I have understood much of it yet, but I had taken a long time to piece enough together to do some useful stuff. 

Nothing wrong with using deprecated code, by the way. If some quick and dirty code that needs no future maintenance is what you need, you can of course use #pragma warning(disable:4996), which will disable the errors. However, the examples here will be handy when the older functions are completely phased out.

One of the resources that kickstarted my journey of pain and torment was Dranger's tutorial (http://dranger.com/ffmpeg/). It explained the concepts in FFmpeg really well. Having said that, it's a little old now, but if you're not averse to setting up your examples with older libraries (uses SDL1.2), check it out. 

Also, +1 to those wonderful people on StackOverflow and irc.lc/freenode/ffmpeg who helped me along the way and keeping my sanity hanging on the edge of the cliff. Now, I would like to give back to the community. So here it is: my attempt to add just another piece of example to the heap of examples already found on the internet. The examples here are based on FFmpeg 3.1 and to the best of my knowledge works as intended. (Having said that, the best of my knowledge isn't the best of knowledge.)

Do what you will with it, but I would really appreciate it that you could contribute back to this repository also. Cheers.


List of examples:
- Video encoding
- Audio encoding

Resources that really helped me (but may be slightly old)
- http://dranger.com/ffmpeg/
- 