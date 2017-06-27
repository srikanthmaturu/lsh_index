//
// Created by srikanth on 6/26/17.
//
#include <string>
#include <iostream>
#include <regex>
#include <vector>
#include <fstream>
#include <iterator>
#include <sstream>
using namespace std;

void getQueriesCount(string hamming_distance_results_file, string min_hash_results_file){

    string pattern = "";
    regex reg(pattern);
    ifstream hd_fs(hamming_distance_results_file), mh_fs(min_hash_results_file);
    regex_iterator<string::iterator> hf_it(istreambuf_iterator<char>(hd_fs), istreambuf_iterator<char>()),
            mhf_it(istreambuf_iterator<char>(mh_fs), istreambuf_iterator<char>()), rend;
    uint64_t mbcount, mocount;
    while (hf_it !=rend && mhf_it != rend){
        string hf_match = *hf_it , mh_match = *mhf_it;
        vector<string> hf_match_lines = strtok(hf_match, '\n'), mh_match_lines = strtok(mh_match, '\n');
        if(hf_match_lines[0] == mh_match_lines[0]){
            if(hf_match_lines.size()> 1 && mh_match_lines.size() == 1){
                mocount++;
            }
            if(hf_match_lines.size()> 1 && mh_match_lines.size() > 1){
                uint64_t hd = stoi(strtok(hf_match_lines[1], "  ")[1]), ed = stoi(strtok(mh_match_lines[1], "  ")[1]);
                if(hd <= ed){
                    mbcount++;
                }
            }
        }
    }

    cout << " MBCount " << mbcount << " MOCount " << mocount << endl;

}


int main(int argc, char* argv[]){
    if(argc < 3){
        cout << "Usage: " << argv[1] << " [hd_results_file] " << " [mh_results_file] " << endl;
        return 0;
    }

    getQueriesCount(argv[1], argv[2]);

}