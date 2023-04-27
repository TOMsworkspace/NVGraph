#include<cstdio>
#include<iostream>
#include<fstream>
#include "inc/instruction.h"
#include<unordered_map>
#include<map>
#include<random>
using namespace std;



int main(){


    for(int i = 0; i < 200; i++) cout << (bool)(*access_pattern_explore)(generator) << endl;

    FILE *trace_file = popen("xz -dc ../traces/gat_citeseer_100M.xz", "r");

    std::fstream out_file = std::fstream("./gat_citeseer_100M.txt", std::ios::out);

    class input_instr instr_cur;

    int cnt = 0;

    while(fread(&instr_cur, sizeof(instr_cur), 1, trace_file) ){
        if(cnt == 100000){
            cout << instr_cur << endl;
        }

        if(cnt++ > 500000 && cnt < 800000 ){
            for (uint32_t i=0; i< NUM_INSTR_SOURCES; i++) {

                if(instr_cur.source_memory[i] ){
                    out_file << (instr_cur.source_memory[i] >> 6) % 64 << endl;
                }
            }

            // for (uint32_t i=0; i< NUM_INSTR_DESTINATIONS; i++) {

            //     if(instr_cur.destination_memory[i] ){
            //         out_file << (instr_cur.destination_memory[i] >> 6) % 64 << endl;
            //     }
            // }
        }
        
    }

    out_file.close();

    fclose(trace_file);

    return 0;
}