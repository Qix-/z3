#include "sat_local_search.h"
#include "sat_solver.h"

static bool build_instance(char const * filename, sat::solver& s, sat::local_search& local_search)
{
    char	line[16383];
    int	cur_term;
    // for temperally storage

    std::ifstream infile(filename);
    //if (infile == NULL) //linux
    if (!infile) {
        std::cout << "File not found " << filename << "\n";
        return false;
    }
    infile.getline(line, 16383);
    int num_vars, num_constraints;
    sscanf_s(line, "%d %d", &num_vars, &num_constraints);
    //std::cout << "number of variables: " << num_vars << '\n';
    //std::cout << "number of constraints: " << num_constraints << '\n';


    unsigned_vector coefficients;
    sat::literal_vector lits;

    // process objective function:
    // read coefficents
    infile >> cur_term;
    while (cur_term != 0) {
        coefficients.push_back(cur_term);
        infile >> cur_term;
    }

    // read variables
    infile >> cur_term;
    while (cur_term != 0) {
        lits.push_back(sat::literal(abs(cur_term), cur_term < 0));
        infile >> cur_term;
    }

    if (lits.size() != coefficients.size()) {
        std::cout << "Objective function format error. They have different lenghts.\n";
        return false;
    }

    for (unsigned i = 0; i < lits.size(); ++i) {
        local_search.add_soft(lits[i].var(), coefficients[i]);
    }
    
    // read the constraints, one at a time
    int k;
    for (int c = 0; c < num_constraints; ++c) {        
        lits.reset();
        infile >> cur_term;
        while (cur_term != 0) {
            lits.push_back(sat::literal(abs(cur_term), cur_term > 0));
            infile >> cur_term;
        }
        infile >> k;
        //local_search.add_cardinality(lits.size(), lits.c_ptr(), static_cast<unsigned>(lits.size() - k));
        local_search.add_cardinality(lits.size(), lits.c_ptr(), static_cast<unsigned>(k));
    }

    infile.close();
	
    return true;
}

void tst_sat_local_search(char ** argv, int argc, int& i) {
    if (argc < i + 2) {
        std::cout << "require dimacs file name\n";
        return;
    }
    reslimit limit;
    params_ref params;
    sat::solver solver(params, limit);
    sat::local_search local_search(solver);
    char const* file_name = argv[i + 1];
    ++i;

	int v;
    while (i + 1 < argc) {
        // set other ad hoc parameters.
		if (argv[i + 1][0] == '-' && i + 2 < argc) {
			switch (argv[i + 1][1]) {
			case 's': // seed
				v = atoi(argv[i + 2]);
				local_search.m_config.set_seed(v);
				break;
			case 't': // cutoff_time
				v = atoi(argv[i + 2]);
				local_search.m_config.set_cutoff_time(v);
				break;
			case 'i': // strategy_id
				v = atoi(argv[i + 2]);
				local_search.m_config.set_strategy_id(v);
				break;
			case 'b': // best_known_value
				v = atoi(argv[i + 2]);
				local_search.m_config.set_best_known_value(v);
				break;
			default:
				++i;
				v = -1;
				break;
			}
		}
        ++i;
    }

    if (!build_instance(file_name, solver, local_search)) {
        return;
    }

    //std::cout << "local instance built\n";
    local_search();

    // sat::solver s;
    // populate the sat solver with clauses and cardinality consrtaints from the input
    // call the lookahead solver.
    // TBD
}
