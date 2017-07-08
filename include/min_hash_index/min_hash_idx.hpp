//
// Created by srikanth on 6/13/17.
//
#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <iostream>
#include <random>
#include <chrono>
#include <fstream>
#include <iterator>
#include <algorithm>
#include "xxhash.h"
#include "min_hash_idx_helper.hpp"
#include <boost/serialization/vector.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

namespace min_hash_index{
    template<class uint_type_t, uint64_t w_size_t, uint64_t app_seq_len_t, uint64_t t_lim_t, uint64_t threshold_t, uint64_t n_perms_t, uint64_t n_segs_t=0, uint64_t seg_size_t=0>
    class min_hash_idx {
    public:
        min_hash_idx() = default;
        min_hash_idx(const min_hash_idx &) = default;
        min_hash_idx(min_hash_idx &&) = default;
        min_hash_idx &operator=(const min_hash_idx &) = default;
        min_hash_idx &operator=(min_hash_idx &&) = default;

        min_hash_idx(std::vector<std::string>& keys){
            this->keys = keys;
            double_t input_threshold =  (double_t)threshold_t/(double_t)100;
            if(input_threshold < 1){
                threshold = input_threshold;
            }
            else if(input_threshold == 1){
                threshold = optimal_threshold_1(keys[0].length() + app_seq_len_t, w_size_t, input_threshold, t_lim_t);
                std::cout << "Type 1 threshold (optimal) computed." << std::endl;
            }
            else{
                threshold = optimal_threshold_2(keys[0].length() + app_seq_len_t, w_size_t, input_threshold, t_lim_t);
                std::cout << "Type 2 threshold (relaxed) computed." << std::endl;
            }
            std::cout << "Threshold set to Th: " << threshold << std::endl;

            if(n_segs == 0){
                auto param = optimal_param(threshold, n_perms_t, 20, 15);
                n_segs = param.first;
                seg_size = param.second;
            }else{
                n_segs = n_segs_t;
                seg_size = seg_size_t;
            }
            n_perms = n_segs*seg_size;
            std::cout << "Optimal parameters computed."<< std::endl;
            std::cout << "Number of bands: " << n_segs << " Size of a band: " << seg_size  << " No of perms: " << n_perms << std::endl;
            mh = min_hash(n_perms);
            segments_map.resize(n_segs);
            hash_ranges.resize(n_segs);
            for(uint64_t i=0; i< n_segs; i++){
                auto start = i*seg_size, end = (i+1)*seg_size;
                hash_ranges[i].first = start;
                hash_ranges[i].second = end;
            }
            for(uint_type i=0; i<this->keys.size(); i++){
                insert(i, this->keys[i]);
            }
            /*for(auto seg_map:segments_map) {
                for (auto pair:seg_map) {
                    std::cout << "key: "<< pair.first << " ";
                    for(auto item:pair.second){
                        std::cout << item << " ";
                    }
                    std::cout << std::endl;
                }
                std::cout << std::endl;
            }
            std::cout << std::endl;*/
        }
    public:
        std::vector<std::pair<uint64_t, uint64_t>> hash_ranges;
        uint64_t n_perms = n_perms_t,
                n_segs = n_segs_t,
                seg_size = seg_size_t;
        double threshold = threshold_t/100;
        std::string app_seq =  std::string(app_seq_len_t, 'T');
        //std::string app_seq = "HELLHELLHELLH";
        std::vector<std::string> keys;
        typedef uint_type_t uint_type;
        std::vector<std::unordered_map<uint64_t, std::vector<uint_type>>> segments_map;

        void store_to_file(std::string idx_file){
            std::ofstream idx_file_ofs(idx_file);
            boost::archive::binary_oarchive oa(idx_file_ofs);
            oa << keys;
            oa << segments_map;
            oa << mh;
            oa << hash_ranges;
            oa << n_perms;
            oa << n_segs;
            oa << seg_size;
            oa << app_seq;
        }

        void load_from_file(std::string idx_file){
            std::ifstream idx_file_ifs(idx_file);
            boost::archive::binary_iarchive ia(idx_file_ifs);
            ia >> keys;
            ia >> segments_map;
            ia >> mh;
            ia >> hash_ranges;
            ia >> n_perms;
            ia >> n_segs;
            ia >> seg_size;
            ia >> app_seq;
        }

        std::pair<uint64_t, std::vector<std::string>> match(std::string key){
            key = key + app_seq;
            auto res = std::vector<uint64_t>();
            auto qry_segs_hashes = get_segments_hashes(mh.hashify(key));
            for(uint64_t i=0; i<n_segs; i++){
                auto & seg_map = segments_map[i];
                auto seg_hash = qry_segs_hashes[i];
                /*std::cout << seg_hash << "\n";*/
                if(seg_map.count(seg_hash) != 0){
                    auto& res_v = seg_map[seg_hash];
                    std::for_each(res_v.begin(), res_v.end(), [&](uint_type &key_index){ res.push_back(key_index);});
                }
            }

            std::cout << "Candidates: " << res.size() << std::endl;
            unique_vec(res);
            std::vector<std::string> res_s;
            res_s.reserve(res.size());
            //std::for_each(res.begin(), res.end(), [&](uint64_t key_index){if(mh.compareHashes(keys[key_index] + app_seq, key, threshold)) {res_s.push_back(keys[key_index]);}});
            //std::for_each(res.begin(), res.end(), [&](uint64_t key_index){res_s.push_back(keys[key_index]);});
            std::for_each(res.begin(), res.end(), [&](uint_type key_index){if(uiLevenshteinDistance(keys[key_index] + app_seq, key) <= t_lim_t) {res_s.push_back(keys[key_index]);}});
            std::cout << "Matches: " << res_s.size() << std::endl;
            return {res.size(), res_s};
        }

        void insert(uint_type key_index, std::string key){
            key += app_seq;
            auto segment_hashes = get_segments_hashes(mh.hashify(key));
            for(uint64_t i=0; i < n_segs; i++){
                auto & seg_map = segments_map[i];
                auto hash_key = segment_hashes[i];
                if(seg_map.count(hash_key) == 0){
                    seg_map[hash_key] = std::vector<uint_type>();
                }
                seg_map[hash_key].push_back(key_index);
            }
        }

        std::vector<uint64_t> get_segments_hashes(std::vector<uint64_t> min_hashes){
            std::vector<uint64_t> segments_hashes;
            for(auto hash_range : hash_ranges){
                auto segment_hash = XXH64(&(*(min_hashes.begin() + hash_range.first)), (hash_range.second - hash_range.first)*8, 0x1b873593);
                segments_hashes.push_back(segment_hash);
            }
            return segments_hashes;
        }
    private:
        struct min_hash {
            std::vector<std::pair<uint64_t, uint64_t>> permutations;
            uint64_t seed;
            min_hash(){

            };
            min_hash(uint64_t n_perms, uint64_t seed=0xcc9e2d51){
                this->seed = seed;
                std::mt19937_64 engine(seed);
                permutations.resize(n_perms);
                for(auto it = permutations.begin(); it!=permutations.end(); it++){
                    (*it).first = engine();
                    (*it).second = engine();
                }
            }
            std::vector<uint64_t> hashify(std::string key){
                std::vector<uint64_t> hashes(permutations.size(), UINT64_MAX);
                for(uint64_t i=0; i< (key.size() - w_size_t + 1); i++){
                    /*std::cout << key.substr(i, w_size_t).c_str() ;*/
                    uint64_t token_hash = XXH64(key.substr(i, w_size_t).c_str(), w_size_t, seed);
                    for(uint64_t j=0; j < permutations.size(); j++){
                        auto & perm = permutations[j];
                        uint64_t hash = remap64(token_hash, perm.first, perm.second);
                        /*std::cout << " " << hash;*/
                        if(hashes[j] > hash){
                            hashes[j] = hash;
                        }
                    }
                    /*std::cout << std::endl;*/
                }
                /*for(auto hash:hashes){
                    std::cout << hash << " ";
                }*/
                /*std::cout << std::endl << std::endl;*/
                return hashes;
            }

            std::vector<uint64_t> getHashes(std::string key){
                std::vector<std::string> ktokens;

                for(uint64_t i=0; i<(key.size() - w_size_t) + 1; i++) {
                    ktokens.push_back(key.substr(i, w_size_t));
                }

                std::vector<uint64_t> xxhashes;
                std::for_each(ktokens.begin(), ktokens.end(), [&](std::string token){xxhashes.push_back(XXH64(token.c_str(), w_size_t, seed));});
                return xxhashes;
            }


            bool compareHashes(std::string key, std::string query, std::double_t threshold){
                std::vector<uint64_t> khashes = getHashes(key), qhashes = getHashes(query);
                std::sort(khashes.begin(), khashes.end());
                std::sort(qhashes.begin(), qhashes.end());
                uint64_t count = 0;
                for(auto k_it=khashes.begin(), q_it=qhashes.begin(); k_it != khashes.end() && q_it != qhashes.end(); ){
                    if(*k_it == *q_it){
                        count++;
                        ++k_it;
                        ++q_it;
                    }
                    else if(*k_it < *q_it){
                        ++k_it;
                    }
                    else {
                        ++q_it;
                    }
                }
                double ratio = ((double_t)count)/((double_t)key.size() - w_size_t + 1);
                if( ratio >= threshold){
                    std::cout << "Si: " << key.size() << "  ";
                    std::cout << "St1: " << key << "  ";
                    std::cout << "St2: " << query << "  ";
                    std::cout << "R: " << ratio << " ";
                    std::cout << std::endl ;
                    return true;
                }
                else {
                    return false;
                }
 /*               double n = (((double)count) - (13 - w_size_t + 1)) ,d = ((double)(key1.size() - w_size_t)) ;*/

            }

            bool compareMinHashes(std::string key1, std::string key2, std::double_t threshold){
                std::vector<uint64_t> key1_hashes = hashify(key1), key2_hashes = hashify(key2);
                uint64_t count =0;

                for(uint64_t i=0; i < key1_hashes.size(); i++){
                    if(key1_hashes[i] == key2_hashes[i]){
                        count++;
                    }
                }

                double ratio = ((double_t)count)/((double_t)key1.size());
                if( ratio >= threshold){
                    std::cout << "Si: " << key1.size() << "  ";
                    std::cout << "St1: " << key1 << "  ";
                    std::cout << "St2: " << key2 << "  ";
                    std::cout << "R: " << ratio << " ";
                    std::cout << std::endl ;
                    return true;
                }
                else {
                    return false;
                }
            }

            template<class Archive>
            void serialize(Archive & ar, const unsigned int version){
                ar & permutations;
                ar & seed;
            }
        } mh;
    };
}
