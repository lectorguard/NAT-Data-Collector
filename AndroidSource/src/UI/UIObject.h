#pragma once
#include "memory"

class Application;

// Type Erasure Concept
class UIObject
{
public:
	template <typename T> 
	UIObject(T&& obj) : ui_object(std::make_unique<Model<T>>(std::forward<T>(obj))) {}

	void Draw(Application *app) 
	{                          
		return ui_object->Draw(app);
	}

	struct UIConcept 
	{                                       
		virtual ~UIConcept() {}
		virtual void Draw(Application* app) = 0;
	};

	template<typename T>                                   
	struct Model : UIConcept 
	{
		Model(const T& t) : object(t) {}
		void Draw(Application* app) override 
		{
			return object.Draw(app);
		}
	private:
		T object;
	};

	std::unique_ptr<UIConcept> ui_object;
};