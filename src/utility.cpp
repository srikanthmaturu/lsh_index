//
// Created by srikanth on 6/26/17.
//
#include <string>
#include <iostream>
#include <regex>
#include <vector>
#include <fstream>
#include <list>

using namespace std;


void getQueriesCount(string hamming_distance_results_file, string min_hash_results_file){
    ifstream hd_fs(hamming_distance_results_file), mh_fs(min_hash_results_file);
    list<string> hdrf_lines, mhrf_lines;
    uint64_t mbcount = 0 , mocount = 0, mecount = 0;
    uint64_t batch_size = 1000000;
    uint64_t count = 0;
    while (!hd_fs.eof() || !mh_fs.eof()){
        cout << "Processing a batch.." << count << endl;
        count++;
        for(uint64_t i=0; i<batch_size; i++){
            string hd_fs_line, mh_fs_line;
            if(!hd_fs.eof()){
                getline(hd_fs, hd_fs_line);
                hdrf_lines.push_back(hd_fs_line);
            }

            if(!mh_fs.eof()){
                getline(mh_fs, mh_fs_line);
                mhrf_lines.push_back(mh_fs_line);
            }
        }
	cout << "Read lines" << endl;
        if(hd_fs.eof()){
            hdrf_lines.pop_back();
        }

        if(mh_fs.eof()) {
            mhrf_lines.pop_back();
        }


	cout << "Bottom path removed and saved for use in next cycle" << endl;
        while(hdrf_lines.size()>0 && mhrf_lines.size()>0){
            if(hdrf_lines.front() == mhrf_lines.front()){
                string query = *hdrf_lines.begin();
                hdrf_lines.pop_front();
                mhrf_lines.pop_front();
                vector<string> hd_matches, mh_matches;
                bool hdf = true, mhf = true;
                while(hdf || mhf){
                    if(hdrf_lines.size() > 0 && (*hdrf_lines.begin())[0] != '>'){
                        hd_matches.push_back(*hdrf_lines.begin());
                        hdrf_lines.pop_front();
                    } else {
                        hdf = false;
                    }
                    if(mhrf_lines.size() > 0 && (*mhrf_lines.begin())[0] != '>'){
                        mh_matches.push_back(*mhrf_lines.begin());
                        mhrf_lines.pop_front();
                    } else {
                        mhf = false;
                    }
                }

                if(hd_matches.size()> 0 && mh_matches.size() == 0){
                    mocount++;
                }
                if(hd_matches.size()> 0 && mh_matches.size() > 0){
                    /*cout << hd_matches[0] << " " << hd_matches[0].size()<< endl;
                    cout << mh_matches[0] << " " << hd_matches[0].size() << endl;*/
                    size_t p1 = hd_matches[0].find("  ") + 2 , p2 = mh_matches[0].find("  ") + 2;
                    //cout << " p1 " << p1 << " p2 " << p2 << endl;
                    uint64_t hd = stoi(hd_matches[0].substr(p1)), ed = stoi(mh_matches[0].substr(p2));
                    if(hd > ed){
                        mbcount++;
                    }
                    else if(hd == ed){
                        mecount++;
                    }
                }
            }else {
                cout << "Status: MBCount " << mbcount << " MOCount " << mocount << " MECOUNT "<< mecount << "  HDS:"<< hdrf_lines.size() << "  MHS:" << mhrf_lines.size() << endl;
                break;
            }
        }
	cout << " Batch cycle completed. " << endl;
    }

    cout << " MBCount " << mbcount << " MOCount " << mocount << " MECOUNT "<< mecount << endl;

}


int main(int argc, char* argv[]){
    if(argc < 3){
        cout << "Usage: " << argv[1] << " [hd_results_file] " << " [mh_results_file] " << endl;
        return 0;
    }

    getQueriesCount(argv[1], argv[2]);

}
