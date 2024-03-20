#pragma once

#include "Math/Vector.h"

FVector AvergePosition(const TArray<FVector> Positions) {
	FVector Avg = FVector::Zero();
	for (FVector Position : Positions) {
		Avg += Position;
	}
	return Avg / Positions.Num();
}