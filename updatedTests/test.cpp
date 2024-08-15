#include "malloc_4.h"
#include <string>
#include <stdlib.h>
#include <memory>
#include <stdexcept>
#include <array>
#include <cstdio>
#include <cassert>
#include <iostream>

using namespace std;
#define NDEBUG




#define SHELLSCRIPT "\
#/bin/bash \n\
echo  \"---------------------------------------------------\" \n\
cat /sys/devices/system/node/node*/meminfo | fgrep HugePages_ \
"


string exec(const char* cmd) {
     array<char, 128> buffer;
     string result;
     unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw  runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

int getfreepages(){
    string commandres=exec("cat /sys/devices/system/node/node*/meminfo | fgrep HugePages_Free");
    cout<<endl<<commandres<<endl;
    commandres=commandres.substr(commandres.find_first_of(':')+1);

    commandres=commandres.substr(commandres.find_first_not_of(' '),commandres.find_first_of('\n')-1);
    //cout<<atoi(commandres.c_str())<<endl<<endl;
    return atoi(commandres.c_str());
}


int main(){  
      //cout<<"2MB : "<<2*MB<<"  with shift  :"<<(unsigned long)(1<<21)<<endl;
    int init=getfreepages();
             cout<<"---------------------------------------------------\n";
    cout<<" INIT "<<endl;

    
        system(SHELLSCRIPT);//prints current state of free and allocated huge tlbs
                     cout<<"---------------------------------------------------\n\n\n";

    if(init<8){
        cout<<"not enough free tables , try running : sudo sh -c 'echo 30 > /proc/sys/vm/nr_hugepages'   \nthen try again \n";
    }

    const size_t KB=1024;//KB =1024 bytes
    const size_t MB=1024*KB;//KB =1024 KB
    void* ptr=smalloc(5*MB);
    char* test=(char*)ptr;
    test[0]=10;
    test[3*MB]=20;
        cout<<"---------------------------------------------------\n";
        cout<<"|| 2 PAGES ALOC                                  ||"<<endl;
             cout<<"---------------------------------------------------\n";

        assert(getfreepages()==init-2);
     //system(SHELLSCRIPT);//prints current state of free and allocated huge tlbs
    sfree(ptr);
             cout<<"---------------------------------------------------\n";
             cout<<"|| 2 PAGES FREE                                  ||"<<endl;
             cout<<"---------------------------------------------------\n";


        assert(getfreepages()==init);

     //system(SHELLSCRIPT);
     cout<<"---------------------------------------------------\n";
   // cout<<"\n pt2 only with size : 6MB \n"<<endl;

    void* ptr2=scalloc(2,3*MB);
                 cout<<"---------------------------------------------------\n";
             cout<<"|| 4 PAGES ALOC                                  ||"<<endl;
             cout<<"---------------------------------------------------\n";

        assert(getfreepages()==init-4);

     //system(SHELLSCRIPT);

    sfree(ptr2); 
                    cout<<"---------------------------------------------------\n";
            cout<<"|| 4 PAGES FREE                                  ||"<<endl;
            cout<<"---------------------------------------------------\n";

        assert(getfreepages()==init);


     //system(SHELLSCRIPT);

     cout<<"---------------------------------------------------\n";
   // cout<<"\n 2 allocations together : \n";

  ptr=smalloc(5*MB);
    
   

    ptr2=scalloc(2,3*MB);
         cout<<"---------------------------------------------------\n";
    //    cout<<"|| after 2 alloc                                 ||\n\n";
  test=(char*)ptr;
    
    test[MB]=25;
        cout<<"|| 4 PAGES + 1 PAGES ALOC                        ||"<<endl;
             cout<<"---------------------------------------------------\n";

        assert(getfreepages()==init-5);

     //system(SHELLSCRIPT);
    
    sfree(ptr);
    cout<<"---------------------------------------------------\n";
    cout<<"|| 1 PAGE FREE                                   ||"<<endl;
             cout<<"---------------------------------------------------\n";

        assert(getfreepages()==init-4);

    cout<<"---------------------------------------------------\n";
   // cout<<"|| afree ptr (5mb only 1 used)                   ||\n";
  //system(SHELLSCRIPT);

     sfree(ptr2);  
    cout<<"|| 4 PAGES FREE (6mb+ all used cuz calloc)       ||"<<endl;
    cout<<"---------------------------------------------------\n";

        assert(getfreepages()==init);


     //system(SHELLSCRIPT);
     ptr=scalloc(10,MB);//shouldnt make huge pages
    cout<<"---------------------------------------------------\n";
     cout<<"|| 0 PAGES ALOC                                  ||"<<endl;
        cout<<"---------------------------------------------------\n";
                assert(getfreepages()==init);

        sfree(ptr);
    cout<<"---------------------------------------------------\n";
     cout<<"|| 0 PAGES FREE                                  ||"<<endl;
        cout<<"---------------------------------------------------\n";
                assert(getfreepages()==init);
     ptr=smalloc(5*MB);
  
    cout<<"---------------------------------------------------\n";
   // cout<<"|| trying realloc : after allocs                 ||\n";
  test=(char*)ptr;
    
    test[MB]=20;
       
    cout<<"|| 1 PAGES ALOC                                  ||"<<endl;
             cout<<"---------------------------------------------------\n";

        assert(getfreepages()==init-1);

     //system(SHELLSCRIPT);
    
    void* ptr3=srealloc(ptr,9*MB);//5 pages

    if(ptr3!=NULL){
    char* test2=(char*)ptr3;  
    for(int i=5;i<9;i++){
        test2[i*MB+2]=i;
    }
 
    cout<<"---------------------------------------------------\n";
   // cout<<"|| after creating pages                          ||\n";
    cout<<"|| 5 PAGES ALOC                                  ||"<<endl;
             cout<<"---------------------------------------------------\n";

        assert(getfreepages()==init-5);

      //system(SHELLSCRIPT);

    sfree(ptr3);

    cout<<"---------------------------------------------------\n";
    cout<<"|| 5 PAGES FREE                                  ||"<<endl;
             cout<<"---------------------------------------------------\n";

                assert(getfreepages()==init);

     cout<<"---------------------------------------------------\n";
  //system(SHELLSCRIPT);

   
    }else {
        cout<<"got null with 9MB realloc\n";
    }
            cout<<"SUCCEEDED"<<endl;
             cout<<"---------------------------------------------------\n";


    return 0;
}