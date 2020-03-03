#include "stickGameWithOpponent.h"

void StickGameWithOpponent::doAction(uint64_t actionID)
{
	LearningEnvironment::doAction(actionID);

	// if the game is not over
	if (!this->isTerminal()) {
		// Execute the action
		// Get current state
		int currentState = (int)((const PrimitiveType<int>&)(this->remainingSticks.getDataAt(typeid(PrimitiveType<int>), 0)));
		if ((actionID + 1) > currentState) {
			// Illegal move
			this->forbiddenMove = true;
			// and game over
			this->remainingSticks.setDataAt(typeid(PrimitiveType<int>), 0, 0);
			// stop there
			return;
		}
		else {
			// update state
			currentState -= ((int)actionID + 1);
			this->remainingSticks.setDataAt(typeid(PrimitiveType<int>), 0, currentState);
			// if current state is now zero, the player lost
		}

		// Random player's turn
		if (currentState > 0) {
			if (rng.getUnsignedInt64(0, errorRate - 1) != 0 && (currentState - 1) % 4 != 0) {
				currentState -= (currentState - 1) % 4;
			}
			else {
				currentState -= (int)rng.getUnsignedInt64(1, std::min(currentState, 3));
			}

			this->remainingSticks.setDataAt(typeid(PrimitiveType<int>), 0, currentState);
			if (currentState == 0) {
				this->win = true;
			}
		}
	}
}

void StickGameWithOpponent::reset(size_t seed, Learn::LearningMode mode)
{
	// Create seed from seed and mode
	size_t hash_seed = std::hash<size_t>()(seed) ^ std::hash<Learn::LearningMode>()(mode);
	this->rng.setSeed(hash_seed);
	this->remainingSticks.setDataAt(typeid(PrimitiveType<int>), 0, 21);
	this->win = false;
	this->forbiddenMove = false;
}

std::vector<std::reference_wrapper<const DataHandlers::DataHandler>> StickGameWithOpponent::getDataSources()
{
	std::vector<std::reference_wrapper<const DataHandlers::DataHandler>> res = { this->hints, this->remainingSticks };

	return res;
}

double StickGameWithOpponent::getScore() const
{
	if (this->win)
	{
		return 1.0;
	}
	else {
		if (!this->forbiddenMove) {
			return 0.0;
		}
		else {
			return -10.0;
		}
	}
}

bool StickGameWithOpponent::isTerminal() const
{
	return (int)((const PrimitiveType<int>&)(this->remainingSticks.getDataAt(typeid(PrimitiveType<int>), 0))) == 0;
}
