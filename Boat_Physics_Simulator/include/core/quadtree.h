#pragma once
#include <memory>
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <unordered_set>

template<typename T>
class QuadTree
{
public:
	struct BoundsResult
	{
		DirectX::BoundingBox boundingBox;
		size_t depth;
	};

	QuadTree()
	{
	}

	~QuadTree()
	{
	}

	void Create(float xDimension, float yDimension, float zDimension, int elementsPerNode, int maxHeight)
	{
		DirectX::BoundingBox bounds;
		bounds.Center = { 0, 0, 0 };
		bounds.Extents = { xDimension, yDimension, zDimension };

		root = std::make_unique<Node>();
		root->bounds = bounds;
		root->depth = 0;
		root->elementsPerNode = elementsPerNode;
		root->maxHeight = maxHeight;
	}

	void Clear()
	{
		Create(
			root->bounds.Extents.x,
			root->bounds.Extents.y,
			root->bounds.Extents.z,
			root->elementsPerNode,
			root->maxHeight
		);
	}

	const std::vector<BoundsResult> GetTreeBounds() // Starts the treversal
	{
		std::vector<BoundsResult> results;

		GetTreeBounds(root, results);

		return results;
	}

	void AddElement(
		const T element,
		const DirectX::BoundingBox boundingBox
	)
	{
		AddToNode(element, boundingBox, root);
	}

	std::unordered_set<T> GetVisibleElements(const DirectX::BoundingFrustum frustum)
	{
		std::unordered_set<T> elements;

		GetVisibleElements(root, elements, frustum);

		return elements;
	}

private:
	struct Object
	{
		T element;
		DirectX::BoundingBox bounds;
	};

	struct Node
	{
		std::vector<Object> objects;
		DirectX::BoundingBox bounds;
		std::unique_ptr<Node> children[4] = { nullptr };
		size_t depth;
		int elementsPerNode;
		int maxHeight;

		bool IsFull() { return objects.size() >= elementsPerNode; }
		bool IsParent() { return children[0] != nullptr; }
		bool IsLeaf() { return !IsParent(); }
	};

	std::unique_ptr<Node> root;

	void GetTreeBounds(std::unique_ptr<Node>& node, std::vector<BoundsResult>& results)
	{
		BoundsResult result = {};
		result.boundingBox = node->bounds;
		result.depth = node->depth;
		results.emplace_back(result);

		if (node->IsParent())
		{
			for (auto& node : node->children)
			{
				GetTreeBounds(node, results);
			}
		}
	}

	void AddToNode(
		const T element,
		const DirectX::BoundingBox& bounds,
		std::unique_ptr<Node>& node
	)
	{
		bool collision = node->bounds.Intersects(bounds); // Check for bounding volume collision
		if (!collision) // If no collision, then discard the node
		{
			return;
		}

		if (node->IsLeaf()) // Check if collided node is a leaf node
		{
			if (!node->IsFull() || node->depth == node->maxHeight) // Node has space or is at max depth
			{
				// Add object and its bounding volume node
				Object object = {};
				object.element = element;
				object.bounds = bounds;
				node->objects.emplace_back(object);
				return;
			}
			else
			{ // The node is a leaf node, but it is not empty
		  // Expand the current node by adding children nodes
		  // Attempt to concurrently add the already esisting objects and
		  // the new object into the new children
				for (int i = 0; i < 4; ++i)
				{
					node->children[i] = std::make_unique<Node>();

					DirectX::BoundingBox childBoundingBox = {};

					int x = i % 2;
					int z = i / 2;

					float childBoundingSizeX = node->bounds.Extents.x / 2;
					float childBoundingSizeZ = node->bounds.Extents.z / 2;
					childBoundingBox.Extents = { childBoundingSizeX, node->bounds.Extents.y, childBoundingSizeZ };

					childBoundingBox.Center = node->bounds.Center;
					childBoundingBox.Center.x -= childBoundingSizeX;
					childBoundingBox.Center.z -= childBoundingSizeZ;

					childBoundingBox.Center.x += x * 2 * childBoundingSizeX;
					childBoundingBox.Center.z += z * 2 * childBoundingSizeZ;

					node->children[i]->bounds = childBoundingBox;
					node->children[i]->depth = node->depth + 1;
					node->children[i]->elementsPerNode = node->elementsPerNode;
					node->children[i]->maxHeight = node->maxHeight;

					// Old objects
					for (Object& object : node->objects)
					{
						AddToNode(object.element, object.bounds, node->children[i]);
					}
					// New object
					AddToNode(element, bounds, node->children[i]);
				}

				node->objects.clear();
			}
		}
		else
		{ // Current node is parent node
	  // For each children node, recursively call this function with the same
	  // object and bounding volume that were recieved by this function call
			for (auto& node : node->children)
			{
				AddToNode(element, bounds, node);
			}
		}
	}

	void GetVisibleElements(std::unique_ptr<Node>& node, std::unordered_set<T>& elements, DirectX::BoundingFrustum frustum)
	{
		if (node->bounds.Intersects(frustum))
		{
			/* If parent get visible elements of children */
			if (node->IsParent())
			{
				for (auto& node : node->children)
				{
					GetVisibleElements(node, elements, frustum);
				}
			}
			/* If leaf add all elements */
			else
			{
				for (auto& obj : node->objects)
				{
					if (elements.find(obj.element) == elements.end())
					{
						elements.insert(obj.element);
					}
				}
			}
		}
	}
};