//(c) 2016-2020 by Authors
//This file is a part of Flye program.
//Released under the BSD license (see LICENSE file)

#pragma once

#include "overlap.h"


float getAlignmentErrKsw(const OverlapRange& ovlp,
					  	 const DnaSequence& trgSeq,
					  	 const DnaSequence& qrySeq,
					  	 float maxAlnErr);

float getAlignmentErrEdlib(const OverlapRange& ovlp,
					  	   const DnaSequence& trgSeq,
					  	   const DnaSequence& qrySeq,
						   float maxAlnErr,
						   bool useHpc);

struct CigOp
{
	char op;
	int len;
};
float getAlignmentCigarKsw(const DnaSequence& trgSeq, size_t trgBegin, size_t trgLen,
			   			   const DnaSequence& qrySeq, size_t qryBegin, size_t qryLen,
			   			   int matchScore, int misScore, int gapOpen, int gapExtend,
			   			   float maxAlnErr, std::vector<CigOp>& cigarOut);

void decodeCigar(const std::vector<CigOp>& cigar, const DnaSequence& trgSeq, size_t trgBegin,
				 const DnaSequence& qrySeq, size_t qryBegin,
				 std::string& outAlnTrg, std::string& outAlnQry);
