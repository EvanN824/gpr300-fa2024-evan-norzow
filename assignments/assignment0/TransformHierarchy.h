#pragma once
#include <glm/glm.hpp>

namespace EN
{
	struct NodeTransform
	{
		glm::mat4 localTransform;
		glm::mat4 globalTransform;
		unsigned int parentIndex = -1;
	};

	class TransformHierarchy
	{
	public:
		~TransformHierarchy() { delete nodes; }
		TransformHierarchy(unsigned int num);
		 
		NodeTransform* GetNode(unsigned int num);
		void SolveFK();

	private:
		TransformHierarchy() {}
		NodeTransform* nodes;
		unsigned int nodeCount;
	};
}