//
// Created by Srikanth Maturu (srikanthmaturu@outlook.com)on 6/12/2017.
//
#include "min_hash_index/min_hash_idx.hpp"

#include <chrono>
#include <iostream>
#include <cstdint>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <typeinfo>
#include <omp.h>

#include "min_hash_index/xxhash.h"

using namespace std;
using namespace min_hash_index;

using namespace std::chrono;
using timer = std::chrono::high_resolution_clock;

const string index_name = INDEX_NAME;

template<class duration_type=std::chrono::seconds>
struct my_timer{
    string phase;
    time_point<timer> start;

    my_timer() = delete;
    my_timer(string _phase) : phase(_phase) {
        std::cout << "Start phase ``" << phase << "''" << std::endl;
        start = timer::now();
    };
    ~my_timer(){
        auto stop = timer::now();
        std::cout << "End phase ``" << phase << "'' ";
        std::cout << " ("<< duration_cast<duration_type>(stop-start).count() << " ";
        std::cout << " ";
        std::cout << " elapsed)" << std::endl;
    }
};

template<class uint_type_t, uint64_t w_size_t, uint64_t app_seq_len_t, uint64_t t_lim_t, uint64_t threshold_t, uint64_t n_perms_t>
struct idx_file_trait{
    static std::string value(std::string hash_file){
        return hash_file + ".UIT_" + typeid(uint_type_t).name() + "WS_" +to_string(w_size_t)+"_"+ "ASL_" + to_string(app_seq_len_t)+"_" +
               "TL_" +to_string(t_lim_t)+"_"+ "NP_" +to_string(n_perms_t)+"."+index_name;
    }
};


void load_sequences(string sequences_file, vector<string>& sequences){
    ifstream input_file(sequences_file, ifstream::in);

    for(string sequence; getline(input_file, sequence);){
        uint64_t pos;
        if((pos=sequence.find('\n')) != string::npos){
            sequence.erase(pos);
        }
        if((pos=sequence.find('\r')) != string::npos){
            sequence.erase(pos);
        }
        sequences.push_back(sequence);
    }
}

int main(int argc, char* argv[]){
    constexpr uint64_t w_size = W_SIZE;
    constexpr uint64_t app_seq_len = APP_SEQ_LEN;
    constexpr uint64_t t_lim = T_LIM;
    constexpr uint64_t n_perms = N_PERMS;
    constexpr uint64_t threshold = THRESHOLD;
    typedef UINT_TYPE uint_type;
    typedef LSH_INDEX_TYPE mh_index_type;

    if ( argc < 2 ) {
        cout << "Usage: ./" << argv[0] << " sequences_file [query_file]" << endl;
        return 1;
    }

    string sequences_file = argv[1];
    string queries_file = argv[2];
    cout << "SF: " << sequences_file << " QF:" << queries_file << endl;
    string idx_file = idx_file_trait<uint_type, w_size, app_seq_len, t_lim, threshold, n_perms>::value(sequences_file);
    string queries_results_file = idx_file_trait<uint_type, w_size, app_seq_len, t_lim, threshold, n_perms>::value(queries_file) + "_search_results.txt";
    mh_index_type mh_i;

    {
        ifstream idx_ifs(idx_file);
        if ( !idx_ifs.good()){
            auto index_construction_begin_time = timer::now();
            vector<string> sequences;
            load_sequences(sequences_file, sequences);
            {
                //            my_timer<> t("index construction");
//                auto temp = index_type(keys, async);
                cout<< "Index construction begins"<< endl;
                auto temp = mh_index_type(sequences);
                //            std::cout<<"temp.size()="<<temp.size()<<std::endl;
                mh_i = std::move(temp);
            }
            mh_i.store_to_file(idx_file);
            auto index_construction_end_time = timer::now();
            cout<< "Index construction completed." << endl;
            cout << "# total_time_to_construct_index_in_us :- " << duration_cast<chrono::microseconds>(index_construction_end_time-index_construction_begin_time).count() << endl;
        } else {
            cout << "Index already exists. Using the existing index." << endl;
        }

        mh_i.load_from_file(idx_file);

        vector<string> queries;
        load_sequences(queries_file, queries);
        vector< vector< pair<string, uint64_t > > > query_results_vector(queries.size());
        ofstream results_file(queries_results_file);
        auto start = timer::now();

        #pragma omp parallel for
        for(uint64_t i=0; i<queries.size(); i++){
            auto res = mh_i.match(queries[i]);
            uint8_t minED = 100;
            for(size_t j=0; j < res.second.size(); ++j){
                uint64_t edit_distance = uiLevenshteinDistance(queries[i], res.second[j]);
                if(edit_distance < minED){
                    minED = edit_distance;
                    query_results_vector[i].clear();
                }
                else if(edit_distance > minED){
                    continue;
                }
                query_results_vector[i].push_back(make_pair(res.second[j], edit_distance));
            }
            cout << "processed query " << i << endl;
        }

        auto stop = timer::now();
        cout << "# time_per_search_query_in_us = " << duration_cast<chrono::microseconds>(stop-start).count()/(double)queries.size() << endl;
        cout << "# total_time_for_entire_queries_in_us = " << duration_cast<chrono::microseconds>(stop-start).count() << endl;
        cout << "saving results in the results file: " << queries_results_file << endl;

        for(uint64_t i=0; i < queries.size(); i++){
            results_file << ">" << queries[i] << endl;
            cout << "Stored results of " << i << endl;
            for(size_t j=0; j<query_results_vector[i].size(); j++){
                results_file << "" << query_results_vector[i][j].first.c_str() << "  " << query_results_vector[i][j].second << endl;
            }
        }
        results_file.close();
    }
}


