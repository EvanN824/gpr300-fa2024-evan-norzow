#include "TransformHierarchy.h"

EN::TransformHierarchy::TransformHierarchy(unsigned int num)
{
	nodes = new NodeTransform[num];
	nodeCount = num;
}

void EN::TransformHierarchy::SolveFK()
{
	for (int i = 0; i < nodeCount; i++)
	{
		if (nodes[i].parentIndex == -1)
		{
			nodes[i].globalTransform = nodes[i].localTransform;
		}
		else
		{
			nodes[i].globalTransform = nodes[nodes[i].parentIndex].globalTransform * nodes[i].localTransform;
		}
	}
}

EN::NodeTransform* EN::TransformHierarchy::GetNode(unsigned int num)
{
	assert(num < nodeCount && num >= 0);
	return nodes[num];
}