1. Create a drive image with at most 65535 sectors of size 512 bytes each 
   (maximum byte size: 33553920). I will assume it's called demoimage.iso. Make
   sure most of the sectors are zeroes, so it fits inside the 128k of flash
   memory.
   
2. Run: 
        python3 compress.py demoimage.iso > demoimage.h
        
3. Compile readonlydrive project.        
        
        
