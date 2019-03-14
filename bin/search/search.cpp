//
// Copyright (C) 2015-2019 Yahoo Japan Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include	"NGT/Index.h"
#include <fstream>
#include <iostream>
#include <chrono>

class Timer {
public:
    Timer() { start_time = chrono::high_resolution_clock::now(); }

    double elapsed_seconds() {
      auto end_time = chrono::high_resolution_clock::now();
      auto elapsed = chrono::duration_cast<chrono::duration<double>>(end_time - start_time);
      return elapsed.count();
    }

private:
    chrono::high_resolution_clock::time_point start_time;
};

int
main(int argc, char **argv)
{
  string	indexFile	= "onng-index";	// Index.
  string	query		= "sift_query.tsv";	// Query file.
  int		size		= 10;		// The number of resultant objects.
  float		radius		= FLT_MAX;	// Radius of search range.
  float		epsilon		= 0.1;		// Epsilon to expand explored range.

  const size_t nq = 10000;
  uint32_t gt[nq];
  {
    std::cout << "Load groundtruths...\n";
    std::ifstream gt_input("../rl_hnsw/notebooks/data/SIFT100K/test_gt.ivecs", std::ios::binary);
    uint32_t dim = 0;
    for (size_t i = 0; i < nq; i++){
      gt_input.read((char *) &dim, sizeof(uint32_t));
      if (dim != 1) {
        std::cout << "file error\n";
        exit(1);
      }
      gt_input.read((char *) (gt + dim*i), dim * sizeof(int));
    }
  }
  float counter = 0.0;
  float time = 0.0;
  float dcs = 0.0;
  try {
    NGT::Index	index(indexFile);		// open the specified index.
    ifstream	is(query);			// open a query file.
    if (!is) {
      cerr << "Cannot open the specified file. " << query << endl;
      return 1;
    }

    string line;
    for (size_t i = 0; i < nq; i++){
      getline(is, line);   		// read  a query object from a query file.
      NGT::Object *query = 0;
      {
	    vector<string> tokens;
	    NGT::Common::tokenize(line, tokens, " \t");      // split a string into words by the separators.
	    // create a vector from the words.
	    vector<double> obj;
	    for (vector<string>::iterator ti = tokens.begin(); ti != tokens.end(); ++ti)
	      obj.push_back(NGT::Common::strtol(*ti));

	    // allocate query object.
	    query = index.allocateObject(obj);
      }
      // set search prameters.
      NGT::SearchContainer sc(*query);		// search parametera container.
      NGT::ObjectDistances objects;		// a result set.
      sc.setResults(&objects);			// set the result set.
      sc.setSize(size);				// the number of resultant objects.
      sc.setRadius(radius);			// search radius.
      sc.setEpsilon(epsilon);			// set exploration coefficient.

      Timer query_time;
      index.search(sc);
      time += query_time.elapsed_seconds();
      dcs += sc.distanceComputationCount;

      // output resultant objects.
      counter += (objects[0].id-1) == gt[i];
      index.deleteObject(query);
    }
  } catch (NGT::Exception &err) {
    cerr << "Error " << err.what() << endl;
    return 1;
  } catch (...) {
    cerr << "Error" << endl;
    return 1;
  }
  std::cout << "Counter: " << counter / nq << std::endl;
  std::cout << "Time: " << 1e6 * time / nq << std::endl;
  std::cout << "dcs: " << dcs / nq << std::endl;

  return 0;
}


