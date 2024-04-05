#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace EN
{
	struct NodeTransform
	{
		glm::vec3 localPosition = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::quat localRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		glm::vec3 localScale = glm::vec3(1.0f, 1.0f, 1.0f);
		glm::mat4 globalTransform;
		unsigned int parentIndex = -1;

		glm::mat4 modelMatrix() const {
			glm::mat4 m = glm::mat4(1.0f);
			m = glm::scale(m, localScale);
			m *= glm::mat4_cast(localRotation);
			m = glm::translate(m, localPosition);
			return m;
		}

		void setValue(glm::vec3 pos, glm::vec3 scale, unsigned int index)
		{
			localPosition = pos;
			localScale = scale;
			parentIndex = index;
		}
	};

	class TransformHierarchy
	{
	public:
		~TransformHierarchy() { delete nodes; }
		TransformHierarchy(unsigned int num);
		void SolveFK();

		NodeTransform* nodes;
		unsigned int nodeCount;
	private:
		TransformHierarchy() {}
	};
}