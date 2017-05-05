#include <iostream>
#include <ctime>
using namespace std;

class State
{
public:
	int last_machine;
	int time_block;
	int cur_process;

	State(int cm = 0, int cp = 0, int tb = 0) :last_machine(cm), cur_process(cp), time_block(tb) {}
};

class Particle
{
public:
	int* cur_pos;
	int evaluation;
	int* pbest_ind;
	int pbest_ind_evaluation;

	Particle();
	void evaluate();
	void update_pbest();
	void evolve();

	static int product_num;
	static int machine_num;
	static int max_time;
	static int* pbest_group;
	static int pbest_group_evaluation;

	static double rand_swap;
	static double rand_history_impact;
	static double rand_group_impact;
};
int Particle::product_num = 0;
int Particle::machine_num = 0;
int Particle::max_time = 0;
int* Particle::pbest_group = NULL;
int Particle::pbest_group_evaluation = INT_MAX;
/// ----------- CONSTANT SETTING ------------ ///
double Particle::rand_swap = 0.9;
double Particle::rand_history_impact = 0.1;
double Particle::rand_group_impact = 0.1;

class Process
{
public:
	int mach_num;
	int time;
	Process(int n = 0, int t = 0) :mach_num(n), time(t) {}

	friend void Particle::evaluate();

	static Process** table;
};
Process** Process::table = NULL;

Particle::Particle()
{
	cur_pos = new int[machine_num*product_num];
	evaluation = 0;
	pbest_ind = new int[machine_num*product_num];
	pbest_ind_evaluation = INT_MAX;

	for (int i = 0; i < machine_num*product_num; i++)
	{
		cur_pos[i] = 0;
	}
	// random initialize
	for (int i = 1; i <= product_num; i++)
	{
		for (int j = 0; j < machine_num; j++)
		{
			int r = rand() % (machine_num*product_num);
			while (cur_pos[r] != 0)
			{
				r++;
				if (r >= machine_num*product_num) r = 0;
			}
			cur_pos[r] = i;
			pbest_ind[r] = i;
		}
	}

	this->evaluate();
	this->update_pbest();
}

void Particle::evaluate()
{
	int** run = new int*[machine_num + 1];		// Gantt Graph
	State *state = new State[product_num + 1];	// product state
	int* last_block = new int[machine_num + 1];	// machines : the last one of block+1

	for (int i = 1; i <= machine_num; i++)
	{
		run[i] = new int[max_time + 1];
		run[i][0] = 0;
		last_block[i] = 0;
	}
	for (int i = 1; i <= product_num; i++)
	{
		state[i].last_machine = Process::table[i][0].mach_num;
	}

	// ---- compute ---- //
	for (int i = 0; i < machine_num*product_num; i++)
	{
		int cur_product = cur_pos[i];
		int cur_proce = state[cur_product].cur_process;
		int cur_mach = Process::table[cur_product][cur_proce].mach_num;
		int time_needed = Process::table[cur_product][cur_proce].time;

		int cur_block = state[cur_product].time_block + 1;
		int cur_last_block = last_block[cur_mach];

		// fill in
		while (1)
		{
			int available = 1; // check whether cur_block is available
			for (int j = 0; j < time_needed; j++)
			{
				if (cur_block + j <= cur_last_block)
				{
					if (run[cur_mach][cur_block + j] != 0)
					{
						cur_block += j;
						available = 0;
						break;
					}
				}
				else break;
			}
			if (available) // fill
			{
				for (int j = 0; j < time_needed; j++)
				{
					run[cur_mach][cur_block + j] = cur_product;
				}
				break;
			}
			while (run[cur_mach][cur_block] != 0 && cur_block <= cur_last_block) cur_block++; // move
		}

		// update
		if (cur_last_block < cur_block)
		{
			cur_last_block++;
			while (cur_last_block < cur_block)
			{
				run[cur_mach][cur_last_block] = 0;
				cur_last_block++;
			}
			last_block[cur_mach] = cur_block + time_needed - 1;
		}

		state[cur_product].time_block = cur_block + time_needed - 1;
		state[cur_product].last_machine = cur_mach;
		state[cur_product].cur_process++;
	}

	// ---- sum ---- //
	int max_time = 0;
	for (int i = 1; i <= product_num; i++)
	{
		if (max_time < state[i].time_block) max_time = state[i].time_block;
	}

	evaluation = max_time;

	for (int i = 1; i <= machine_num; i++) delete[] run[i];
	delete[] last_block;
	delete[] run;
	delete[] state;
}

void Particle::update_pbest()
{
	// ---- update pbest (ind / group) ---- //
	if (pbest_ind_evaluation > evaluation)
	{
		pbest_ind_evaluation = evaluation;
		for (int i = 0; i < machine_num*product_num; i++)
		{
			pbest_ind[i] = cur_pos[i];
		}

		// group
		if (pbest_group_evaluation > evaluation)
		{
			pbest_group_evaluation = evaluation;
			for (int i = 0; i < machine_num*product_num; i++)
			{
				pbest_group[i] = cur_pos[i];
			}
		}
	}
}

void Particle::evolve()
{
	double m = (double)(rand() % 1000) / 1000.0;
	if (m < rand_swap)
	{
		int a = rand() % (machine_num*product_num);
		int b = rand() % (machine_num*product_num);
		while (cur_pos[a] == cur_pos[b]) b = rand() % (machine_num*product_num);

		int temp = cur_pos[a];
		cur_pos[a] = cur_pos[b];
		cur_pos[b] = temp;
	}

	m = (double)(rand() % 1000) / 1000.0;
	if (m < rand_history_impact)
	{
		int *group_divided = new int[product_num + 1];
		for (int i = 1; i <= product_num; i++)
		{
			group_divided[i] = rand() % 2;
		}

		int *pos_impacted = new int[machine_num*product_num];
		for (int i = 0, j = 0; i < machine_num*product_num; i++)
		{
			if (group_divided[cur_pos[i]])
			{
				pos_impacted[j] = cur_pos[i];
				j++;
			}
			if (!group_divided[pbest_ind[i]])
			{
				pos_impacted[j] = pbest_ind[i];
				j++;
			}
		}

		for (int i = 0; i < machine_num*product_num; i++)
		{
			cur_pos[i] = pos_impacted[i];
		}

		delete[] group_divided;
		delete[] pos_impacted;
	}

	m = (double)(rand() % 1000) / 1000.0;
	if (m < rand_group_impact)
	{
		int *group_divided = new int[product_num + 1];
		for (int i = 1; i <= product_num; i++)
		{
			group_divided[i] = rand() % 2;
		}

		int *pos_impacted = new int[machine_num*product_num];
		for (int i = 0, j = 0; i < machine_num*product_num; i++)
		{
			if (group_divided[cur_pos[i]])
			{
				pos_impacted[j] = cur_pos[i];
				j++;
			}
			if (!group_divided[pbest_group[i]])
			{
				pos_impacted[j] = pbest_group[i];
				j++;
			}
		}

		for (int i = 0; i < machine_num*product_num; i++)
		{
			cur_pos[i] = pos_impacted[i];
		}

		delete[] group_divided;
		delete[] pos_impacted;
	}
}

int main()
{
	freopen("1.txt", "r", stdin);
	srand((int)time(0));
	cout << "Please enter num of product" << endl;
	int product_num = 0;
	cin >> product_num;
	cout << "Please enter num of machine" << endl;
	int machine_num = 0;
	cin >> machine_num;

	Particle::machine_num = machine_num;
	Particle::product_num = product_num;
	Particle::pbest_group = new int[machine_num*product_num];

	cout << "Please enter time table" << endl;
	Process::table = new Process*[product_num + 1];
	for (int i = 1; i <= product_num; i++)
	{
		Process::table[i] = new Process[machine_num];
		for (int j = 0; j < machine_num; j++)
		{
			cin >> Process::table[i][j].time;
			Particle::max_time += Process::table[i][j].time;
		}
	}
	cout << "Please enter sequence table" << endl;
	for (int i = 1; i <= product_num; i++)
	{
		for (int j = 0; j < machine_num; j++)
		{
			cin >> Process::table[i][j].mach_num;
		}
	}

	// ------- INPUT ---------- //

	Particle* particles = new Particle[product_num];

	// Set iteration correspoundingly
	long long iteration = product_num*machine_num * 100;
	for (long long i = 0; i < iteration; i++)
	{
		for (int j = 0; j < product_num; j++)
		{
			particles[j].evolve();
			particles[j].evaluate();
			particles[j].update_pbest();
		}

		// Ä£ÄâÍË»ðË¼Ïë
		Particle::rand_swap = 0.9 - 0.5*i*i / (iteration*iteration);
		Particle::rand_history_impact = 0.1 + 0.5*i*i / (iteration*iteration);
		Particle::rand_group_impact = 0.1 + 0.5*i*i / (iteration*iteration);
	}

	cout << "Least time needed: " << Particle::pbest_group_evaluation << endl;
	cout << "Correspounding sequence:" << endl;
	for (int i = 0; i < machine_num*product_num; i++)
	{
		cout << Particle::pbest_group[i] << " ";
	}
	cout << endl;
	system("pause");
}

