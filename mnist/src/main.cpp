#include <iostream>
#include <numeric>

#include <gegelati.h>

#include "mnist.h"

/**
* \brief Function printing the statistics of the classification.
*/
void printClassifStatsTable(MNIST& mnistLE, std::multimap<double, const TPG::TPGVertex*>& result) {
	// Print table of classif of the best
	TPG::TPGExecutionEngine tee(NULL);

	// Get best root		
	auto iterResults = result.begin();
	std::advance(iterResults, result.size() - 1);
	auto bestRoot = iterResults->second;

	// Change the MODE of mnist
	mnistLE.reset(0, Learn::LearningMode::TESTING);

	// Fill the table
	uint64_t classifTable[10][10] = { 0 };
	uint64_t nbPerClass[10] = { 0 };

	const int TOTAL_NB_IMAGE = 1000;
	for (int nbImage = 0; nbImage < TOTAL_NB_IMAGE; nbImage++) {
		// Get answer
		uint8_t currentLabel = mnistLE.getCurrentImageLabel();
		nbPerClass[currentLabel]++;

		// Execute
		auto path = tee.executeFromRoot(*bestRoot);
		const TPG::TPGAction* action = (const TPG::TPGAction*)path.at(path.size() - 1);
		uint8_t actionID = (uint8_t)action->getActionID();

		// Increment table
		classifTable[currentLabel][actionID]++;

		// Do action (to trigger image update)
		mnistLE.doAction(action->getActionID());
	}

	// Print the table
	printf("\t");
	for (int i = 0; i < 10; i++) {
		printf("%d\t", i);
	}
	printf("Nb\n");
	for (int i = 0; i < 10; i++) {
		printf("%d\t", i);
		for (int j = 0; j < 10; j++) {
			printf("%2.1f\t", 100.0 * (double)classifTable[i][j] / (double)nbPerClass[i]);
		}
		printf("%lld\n", nbPerClass[i]);
	}
}

int main() {
	std::cout << "Start MNIST application." << std::endl;

	// Create the instruction set for programs
	Instructions::Set set;
	auto minus = [](double a, double b)->double {return a - b; };
	auto add = [](double a, double b)->double {return a + b; };
	auto mult = [](double a, double b)->double {return a * b; };
	auto div = [](double a, double b)->double {return a / b; };
	auto max = [](double a, double b)->double {return std::max(a, b); };
	auto ln = [](double a, double b)->double {return std::log(a); };
	auto exp = [](double a, double b)->double {return std::exp(a); };


	set.add(*(new Instructions::LambdaInstruction<double>(minus)));
	set.add(*(new Instructions::LambdaInstruction<double>(add)));
	set.add(*(new Instructions::LambdaInstruction<double>(mult)));
	set.add(*(new Instructions::LambdaInstruction<double>(div)));
	set.add(*(new Instructions::LambdaInstruction<double>(max)));
	set.add(*(new Instructions::LambdaInstruction<double>(exp)));
	set.add(*(new Instructions::LambdaInstruction<double>(ln)));

	// Set the parameters for the learning process.
	// (Controls mutations probability, program lengths, and graph size
	// among other things)
	Learn::LearningParameters params;
	params.mutation.tpg.maxInitOutgoingEdges = 3;
	params.mutation.tpg.nbRoots = 500;
	params.mutation.tpg.pEdgeDeletion = 0.7;
	params.mutation.tpg.pEdgeAddition = 0.7;
	params.mutation.tpg.pProgramMutation = 0.2;
	params.mutation.tpg.pEdgeDestinationChange = 0.1;
	params.mutation.tpg.pEdgeDestinationIsAction = 0.5;
	params.mutation.tpg.maxOutgoingEdges = 5;
	params.mutation.prog.pAdd = 0.5;
	params.mutation.prog.pDelete = 0.5;
	params.mutation.prog.pMutate = 1.0;
	params.mutation.prog.pSwap = 1.0;
	params.mutation.prog.maxProgramSize = 20;
	params.maxNbActionsPerEval = 500;
	params.nbIterationsPerPolicyEvaluation = 1;
	params.ratioDeletedRoots = 0.90;
	params.archiveSize = 500;
	params.archivingProbability = 0.01;

	// Instantiate the LearningEnvironment
	MNIST mnistLE;

	// Instantiate and init the learning agent
	Learn::LearningAgent la(mnistLE, set, params);
	la.init();

	// Create an exporter for all graphs
	Exporter::TPGGraphDotExporter dotExporter("out_000.dot", la.getTPGGraph());

	// Train for 300 generations
	printf("Gen\tNbVert\tMin\tAvg\tMax\n");
	for (int i = 0; i < 300; i++) {
		char buff[12];
		sprintf(buff, "out_%03d.dot", i);
		dotExporter.setNewFilePath(buff);
		dotExporter.print();
		std::multimap<double, const TPG::TPGVertex*> result;
		result = la.evaluateAllRoots(i, Learn::LearningMode::VALIDATION);
		auto iter = result.begin();
		double min = iter->first;
		std::advance(iter, result.size() - 1);
		double max = iter->first;
		double avg = std::accumulate(result.begin(), result.end(), 0.0,
			[](double acc, std::pair<double, const TPG::TPGVertex*> pair)->double {return acc + pair.first; });
		avg /= result.size();
		printf("%3d\t%4lld\t%1.2lf\t%1.2lf\t%1.2lf\n", i, la.getTPGGraph().getNbVertices(), min, avg, max);

		printClassifStatsTable(mnistLE, result);

		la.trainOneGeneration(i);
	}

	// Keep best policy
	la.keepBestPolicy();
	dotExporter.setNewFilePath("out_best.dot");
	dotExporter.print();

	// cleanup
	for (int i = 0; i < set.getNbInstructions(); i++) {
		delete (&set.getInstruction(i));
	}

	return 0;
}