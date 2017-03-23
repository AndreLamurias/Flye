//(c) 2016 by Authors
//This file is a part of ABruijn program.
//Released under the BSD license (see LICENSE file)

#include <deque>

#include "graph_processing.h"
#include "logger.h"

void GraphProcessor::simplify()
{
	_outdatedEdges.clear();
	this->trimTips();
	this->unrollLoops();
	//this->condenceEdges();
	this->updateEdgesMultiplicity();
}

void GraphProcessor::unrollLoops()
{
	auto unrollEdge = [](GraphEdge& loopEdge)
	{
		GraphEdge* prevEdge = nullptr;
		for (auto& inEdge : loopEdge.nodeLeft->inEdges)
		{
			if (inEdge != &loopEdge) prevEdge = inEdge;
		}
		GraphEdge* nextEdge = nullptr;
		for (auto& outEdge : loopEdge.nodeLeft->outEdges)
		{
			if (outEdge != &loopEdge) nextEdge = outEdge;
		}

		/*
		Logger::get().debug() << "In";
		for (auto seg : prevEdge->seqSegments)
		{
			Logger::get().debug() << seg.seqId << " " << seg.start << " " << seg.end;
		}
		Logger::get().debug() << "Out";
		for (auto seg : nextEdge->seqSegments)
		{
			Logger::get().debug() << seg.seqId << " " << seg.start << " " << seg.end;
		}*/

		auto growingSeqs = prevEdge->seqSegments;
		std::vector<SequenceSegment> updatedSeqs;
		for (auto& seq : growingSeqs)
		{
			while (true)
			{
				bool updated = false;
				for (auto& otherSeq : loopEdge.seqSegments)
				{
					if (seq.seqId != otherSeq.seqId ||
						seq.end != otherSeq.start) continue;

					updated = true;
					seq.end = otherSeq.end;
					break;
				}

				bool complete = false;
				for (auto& otherSeq : nextEdge->seqSegments)
				{
					if (seq.seqId != otherSeq.seqId ||
						seq.end != otherSeq.start) continue;

					complete = true;
					break;
				}

				if (complete)
				{
					updatedSeqs.push_back(seq);
					break;
				}
				if (!updated) break;
			}
		}
		if (!updatedSeqs.empty())
		{
			prevEdge->seqSegments = updatedSeqs;
			return true;
		}
		else
		{
			Logger::get().debug() << "Can't unroll";
			return false;
		}
	};

	std::unordered_set<GraphEdge*> toRemove;
	for (GraphEdge* edge : _graph.iterEdges())
	{
		if (edge->nodeLeft == edge->nodeRight &&
			edge->nodeLeft->inEdges.size() == 2 &&
			edge->nodeLeft->outEdges.size() == 2 &&
			edge->nodeLeft->neighbors().size() == 3) 	//including itself
		{
			if (unrollEdge(*edge))
			{
				Logger::get().debug() << "Unroll " << edge->edgeId.signedId();
				toRemove.insert(edge);
			}
		}
	}

	for (auto edge : toRemove)	{_graph.removeEdge(edge);};
}

void GraphProcessor::trimTips()
{

	std::unordered_set<GraphEdge*> toRemove;
	for (GraphEdge* edge : _graph.iterEdges())
	{
		int prevDegree = edge->nodeLeft->inEdges.size();
		int nextDegree = edge->nodeRight->outEdges.size();
		if (edge->length() < _tipThreshold && 
			(prevDegree == 0 || nextDegree == 0))
		{
			GraphNode* toFix = (prevDegree != 0) ? edge->nodeLeft : 
								edge->nodeRight;
			toRemove.insert(edge);

			//label all adjacent repetitive edges
			std::unordered_set<GraphNode*> visited;
			std::deque<GraphNode*> queue;
			queue.push_back(toFix);
			while (!queue.empty())
			{
				GraphNode* node = queue.back();
				queue.pop_back();
				if (visited.count(node)) continue;
				visited.insert(node);

				for (auto& edge : node->inEdges) 
				{
					if (edge->isRepetitive() &&
						edge->nodeLeft != edge->nodeRight)
					{
						_outdatedEdges.insert(edge->edgeId);
						queue.push_back(edge->nodeLeft);
					}
				}
				for (auto& edge : node->outEdges) 
				{
					if (edge->isRepetitive() &&
						edge->nodeLeft != edge->nodeRight)
					{
						_outdatedEdges.insert(edge->edgeId);
						queue.push_back(edge->nodeRight);
					}
				}
			}
		}
	}

	Logger::get().debug() << toRemove.size() << " tips removed";
	for (auto edge : toRemove)	{_graph.removeEdge(edge);};
}

/*
void GraphProcessor::condenceEdges()
{
	auto collapseEdges = [this](const GraphPath& edges)
	{
		std::list<SequenceSegment> growingSeqs(edges.front()->seqSegments.begin(),
											   edges.front()->seqSegments.end());
		assert(edges.size() > 1);
		for (size_t i = 1; i < edges.size(); ++i)
		{
			for (auto prevSeg = growingSeqs.begin(); 
				 prevSeg != growingSeqs.end(); )
			{
				bool continued = false;
				for (auto& nextSeg : edges[i]->seqSegments)
				{
					if (prevSeg->seqId == nextSeg.seqId &&
						prevSeg->end == nextSeg.start)
					{
						continued = true;
						prevSeg->end = nextSeg.end;
					}
				}
				if (!continued)
				{
					prevSeg = growingSeqs.erase(prevSeg);
				}
				else
				{
					++prevSeg;
				}
			}
			if (growingSeqs.empty())
			{
				Logger::get().debug() << "Can't collape edge";
				return;
			}
		}

		///New edge
		GraphEdge* newEdge = _graph.addEdge(GraphEdge(edges.front()->nodeLeft,
								 		edges.back()->nodeRight,
								 		FastaRecord::Id(_graph._nextEdgeId++)));
		std::copy(growingSeqs.begin(), growingSeqs.end(),
				  std::back_inserter(newEdge->seqSegments));
		newEdge->multiplicity = std::max((int)growingSeqs.size(), 2);
		Logger::get().debug() << "Added " << newEdge->edgeId.signedId();
		_outdatedEdges.insert(newEdge->edgeId);
		
		for (GraphEdge* edge : edges)
		{
			Logger::get().debug() << "Removed " << edge->edgeId.signedId();
			_outdatedEdges.erase(edge->edgeId);
			_graph.removeEdge(edge);
		}
	};

	std::vector<GraphPath> toCollapse;
	std::unordered_set<FastaRecord::Id> usedDirections;
	for (auto& node : _graph.iterNodes())
	{
		if (!node->isBifurcation()) continue;

		for (auto& direction : node->outEdges)
		{
			if (usedDirections.count(direction->edgeId)) continue;
			usedDirections.insert(direction->edgeId);

			GraphNode* curNode = direction->nodeRight;
			GraphPath traversed;
			traversed.push_back(direction);
			while (!curNode->isBifurcation() &&
				   !curNode->outEdges.empty())
			{
				traversed.push_back(curNode->outEdges.front());
				curNode = curNode->outEdges.front()->nodeRight;
			}
			usedDirections.insert(traversed.back()->edgeId.rc());
			
			if (traversed.size() > 1)
			{
				toCollapse.emplace_back(std::move(traversed));
			}
		}
	}

	for (auto& edges : toCollapse)
	{
		GraphPath complEdges = _graph.complementPath(edges);
		collapseEdges(edges);
		collapseEdges(complEdges);
	}
}*/

void GraphProcessor::generateContigs()
{
	std::unordered_map<FastaRecord::Id, size_t> edgeIds;
	size_t nextEdgeId = 0;
	auto pathToId = [&edgeIds, &nextEdgeId](GraphPath path)
	{
		if (!edgeIds.count(path.front()->edgeId))
		{
			edgeIds[path.front()->edgeId] = nextEdgeId;
			edgeIds[path.back()->edgeId.rc()] = nextEdgeId + 1;
			nextEdgeId += 2;
		}
		return FastaRecord::Id(edgeIds[path.front()->edgeId]);
	};
	
	for (auto& node : _graph.iterNodes())
	{
		if (!node->isBifurcation()) continue;

		for (auto& direction : node->outEdges)
		{
			GraphNode* curNode = direction->nodeRight;
			GraphPath traversed;
			traversed.push_back(direction);

			while (!curNode->isBifurcation() &&
				   !curNode->outEdges.empty())
			{
				traversed.push_back(curNode->outEdges.front());
				curNode = curNode->outEdges.front()->nodeRight;
			}
			
			bool resolvedRepeat = true;
			for (auto& edge : traversed)
			{
				if (edge->multiplicity != 0) resolvedRepeat = false;
			}
			if (resolvedRepeat) continue;

			FastaRecord::Id edgeId = pathToId(traversed);
			_contigs.emplace_back(traversed, edgeId);
		}
	}
}

void GraphProcessor::outputContigsFasta(const std::string& filename)
{
	static const size_t FASTA_SLICE = 80;

	std::ofstream fout(filename);
	if (!fout.is_open()) throw std::runtime_error("Can't open " + filename);
	
	for (auto& contig : _contigs)
	{
		if (!contig.id.strand()) continue;

		std::string contigSequence;
		for (auto& edge : contig.path) 
		{
			if (!edge->seqSegments.empty())
			{
				const SequenceSegment& seg = edge->seqSegments.front();
				const std::string& edgeSeq = !edge->readSequence ? 
											 _asmSeqs.getSeq(seg.seqId) : 
											 _readSeqs.getSeq(seg.seqId);
				contigSequence += edgeSeq.substr(seg.start, seg.end - seg.start);
			}
			else
			{
				Logger::get().warning() << "Edge without sequence!";
			}
		}

		fout << ">edge_" << contig.id.signedId() << std::endl;
		for (size_t c = 0; c < contigSequence.length(); c += FASTA_SLICE)
		{
			fout << contigSequence.substr(c, FASTA_SLICE) << std::endl;
		}
	}
}

void GraphProcessor::outputContigsGraph(const std::string& filename)
{
	std::ofstream fout(filename);
	if (!fout.is_open()) throw std::runtime_error("Can't open " + filename);

	fout << "digraph {\n";
	
	///re-enumerating helper functions
	std::unordered_map<GraphNode*, int> nodeIds;
	int nextNodeId = 0;
	auto nodeToId = [&nodeIds, &nextNodeId](GraphNode* node)
	{
		if (!nodeIds.count(node))
		{
			nodeIds[node] = nextNodeId++;
		}
		return nodeIds[node];
	};

	const std::string COLORS[] = {"red", "darkgreen", "blue", "goldenrod", 
								  "cadetblue", "darkorchid", "aquamarine1", 
								  "darkgoldenrod1", "deepskyblue1", 
								  "darkolivegreen3"};
	std::unordered_map<FastaRecord::Id, size_t> colorIds;
	size_t nextColorId = 0;
	auto idToColor = [&colorIds, &nextColorId, &COLORS](FastaRecord::Id id)
	{
		if (!id.strand()) id = id.rc();
		if (!colorIds.count(id))
		{
			colorIds[id] = nextColorId;
			nextColorId = (nextColorId + 1) % 10;
		}
		return COLORS[colorIds[id]];
	};
	/////////////

	for (auto& contig : _contigs)
	{
		int32_t contigLength = 0;
		for (auto& edge : contig.path) contigLength += edge->length();

		if (contig.path.front()->isRepetitive())
		{
			std::string color = idToColor(contig.id);

			fout << "\"" << nodeToId(contig.path.front()->nodeLeft) 
				 << "\" -> \"" << nodeToId(contig.path.back()->nodeRight)
				 << "\" [label = \"" << contig.id.signedId() << 
				 " " << contigLength << " (" 
				 << contig.path.front()->multiplicity << ")\", color = \"" 
				 << color << "\" " << " penwidth = 3] ;\n";
		}
		else
		{
			fout << "\"" << nodeToId(contig.path.front()->nodeLeft) 
				 << "\" -> \"" << nodeToId(contig.path.back()->nodeRight)
				 << "\" [label = \"" << contig.id
				 << " " << contigLength << "\", color = \"black\"] ;\n";
		}
	}

	fout << "}\n";

}

void GraphProcessor::updateEdgesMultiplicity()
{
	bool anyChanges = true;
	while (anyChanges)
	{
		anyChanges = false;
		for (auto& node : _graph.iterNodes())
		{
			int numUnknown = 0;
			bool isOut = false;
			int inDegree = 0;
			int outDegree = 0;
			GraphEdge* unknownEdge = nullptr;

			for (auto& outEdge : node->outEdges)
			{
				if (outEdge->nodeLeft == outEdge->nodeRight) continue;

				if (!_outdatedEdges.count(outEdge->edgeId))
				{
					outDegree += outEdge->multiplicity;
				}
				else
				{
					++numUnknown;
					unknownEdge = outEdge;
					isOut = true;
				}
			}
			for (auto& inEdge : node->inEdges)
			{
				if (inEdge->nodeLeft == inEdge->nodeRight) continue;

				if (!_outdatedEdges.count(inEdge->edgeId))
				{
					inDegree += inEdge->multiplicity;
				}
				else
				{
					++numUnknown;
					unknownEdge = inEdge;
					isOut = false;
				}
			}
			int newMultiplicity = isOut ? inDegree - outDegree : 
										  outDegree - inDegree;

			if (numUnknown == 1)
			{
				Logger::get().debug() << "Updated: " 
					<< unknownEdge->edgeId.signedId() << " " 
					<< unknownEdge->multiplicity 
					<< " " << newMultiplicity;

				GraphEdge* complEdge = _graph.complementPath({unknownEdge}).front();
				unknownEdge->multiplicity = newMultiplicity;
				complEdge->multiplicity = newMultiplicity;
				_outdatedEdges.erase(unknownEdge->edgeId);
				_outdatedEdges.erase(complEdge->edgeId);
				anyChanges = true;
			}
		}
	}

	Logger::get().debug() << _outdatedEdges.size() << " edges were not updated!";
	for (auto edge : _outdatedEdges)
	{
		Logger::get().debug() << edge.signedId();
	}
}
