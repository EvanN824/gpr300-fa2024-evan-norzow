#include "TransformHierarchy.h"

EN::TransformHierarchy::TransformHierarchy(unsigned int num)
{
	nodes = new NodeTransform[num];
	nodeCount = num;

	for (unsigned int i = 0; i < num; i++)
	{
		nodes[i] = NodeTransform();
	}
}

void EN::TransformHierarchy::SolveFK()
{
	for (unsigned int i = 0; i < nodeCount; i++)
	{
		if (nodes[i].parentIndex == -1)
		{
			nodes[i].globalTransform = nodes[i].modelMatrix();
		}
		else
		{
			nodes[i].globalTransform = nodes[nodes[i].parentIndex].globalTransform * nodes[i].modelMatrix();
		}
	}
}