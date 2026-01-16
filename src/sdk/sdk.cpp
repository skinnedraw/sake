#define NOMINMAX
#include <Windows.h>
#include <sdk/sdk.h>
#include <sdk/offsets.h>
#include <memory/memory.h>
#include <game/game.h>

std::string rbx::nameable_t::get_name()
{
	std::uint64_t name = memory->read<std::uint64_t>(this->address + Offsets::Instance::Name);

	if (name)
	{
		return memory->read_string(name);
	}

	return "unknown";
}

std::string rbx::nameable_t::get_class_name()
{
	std::uint64_t class_descriptor = memory->read<std::uint64_t>(this->address + Offsets::Instance::ClassDescriptor);
	std::uint64_t class_name = memory->read<std::uint64_t>(class_descriptor + Offsets::Instance::ClassName);

	if (class_name)
	{
		return memory->read_string(class_name);
	}

	return "unknown";
}

std::vector<rbx::instance_t> rbx::interface_t::get_children()
{
	rbx::instance_t* base = static_cast<rbx::instance_t*>(this);

	std::uint64_t start{ memory->read<std::uint64_t>(base->address + Offsets::Instance::ChildrenStart) };
	std::uint64_t end{ memory->read<std::uint64_t>(start + Offsets::Instance::ChildrenEnd) };

	std::vector<rbx::instance_t> children;
	children.reserve(32);

	for (std::uint64_t instance = memory->read<std::uint64_t>(start); instance != end; instance += sizeof(std::shared_ptr<void*>))
	{
		children.emplace_back(memory->read<std::uint64_t>(instance));
	}

	return children;
}

rbx::instance_t rbx::interface_t::find_first_child(std::string_view str)
{
	std::vector<rbx::instance_t> children = this->get_children();

	for (rbx::instance_t& child : children)
	{
		if (child.get_name() == str)
		{
			return child;
		}
	}

	return {};
}

rbx::instance_t rbx::interface_t::find_first_child_by_class(std::string_view str)
{
	std::vector<rbx::instance_t> children = this->get_children();

	for (rbx::instance_t& child : children)
	{
		if (child.get_class_name() == str)
		{
			return child;
		}
	}

	return {};
}

rbx::model_instance_t rbx::player_t::get_model_instance()
{
	return { memory->read<std::uint64_t>(this->address + Offsets::Player::ModelInstance) };
}

std::uint8_t rbx::humanoid_t::get_rig_type()
{
	return { memory->read<std::uint8_t>(this->address + Offsets::Humanoid::RigType) };
}

rbx::primitive_t rbx::part_t::get_primitive()
{
	return { memory->read<std::uint64_t>(this->address + Offsets::BasePart::Primitive) };
}

math::vector3 rbx::primitive_t::get_size()
{
	return memory->read<math::vector3>(this->address + Offsets::BasePart::Size);
}

void rbx::primitive_t::set_size(const math::vector3& size)
{
	memory->write<math::vector3>(this->address + Offsets::BasePart::Size, size);
}

math::vector3 rbx::primitive_t::get_position()
{
	return memory->read<math::vector3>(this->address + Offsets::BasePart::Position);
}

math::matrix3 rbx::primitive_t::get_rotation()
{
	return memory->read<math::matrix3>(this->address + Offsets::BasePart::Rotation);
}

bool rbx::primitive_t::get_can_collide()
{
	std::uint64_t primitive = this->address;
	if (!primitive) return false;
	
	std::uint8_t flags = memory->read<std::uint8_t>(primitive + Offsets::BasePart::PrimitiveFlags);
	return (flags & Offsets::PrimitiveFlags::CanCollide) != 0;
}

bool rbx::primitive_t::set_can_collide(bool enable)
{
	std::uint64_t primitive = this->address;
	if (!primitive) return false;
	
	std::uint8_t flags = memory->read<std::uint8_t>(primitive + Offsets::BasePart::PrimitiveFlags);
	
	if (enable)
		flags |= Offsets::PrimitiveFlags::CanCollide;
	else
		flags &= ~Offsets::PrimitiveFlags::CanCollide;
	
	memory->write<std::uint8_t>(primitive + Offsets::BasePart::PrimitiveFlags, flags);
	return enable;
}

math::vector2 rbx::visualengine_t::get_dimensions()
{
	return memory->read<math::vector2>(this->address + Offsets::VisualEngine::Dimensions);
}

math::matrix4 rbx::visualengine_t::get_viewmatrix()
{
	return memory->read<math::matrix4>(this->address + Offsets::VisualEngine::ViewMatrix);
}

bool rbx::visualengine_t::world_to_screen(const math::vector3& world, math::vector2& out, const math::vector2& dims, const math::matrix4& view)
{
	math::vector4 clip = view.multiply({ world.x, world.y, world.z, 1.0f });

	if (clip.w < 0.1f)
	{
		return false;
	}

	clip.x /= clip.w;
	clip.y /= clip.w;

	out.x = (dims.x * 0.5f * clip.x) + (dims.x * 0.5f);
	out.y = -(dims.y * 0.5f * clip.y) + (dims.y * 0.5f);

	HWND roblox_window = game::wnd;
	if (roblox_window)
	{
		RECT client_rect{};
		POINT client_pos{};
		if (GetClientRect(roblox_window, &client_rect))
		{
			client_pos.x = client_rect.left;
			client_pos.y = client_rect.top;
			ClientToScreen(roblox_window, &client_pos);
			out.x += (float)client_pos.x;
			out.y += (float)client_pos.y;
		}
	}

	return true;
}

math::vector3 rbx::camera_t::get_position()
{
	return memory->read<math::vector3>(this->address + Offsets::Camera::Position);
}

math::matrix3 rbx::camera_t::get_rotation()
{
	return memory->read<math::matrix3>(this->address + Offsets::Camera::Rotation);
}

void rbx::camera_t::write_rotation(const math::matrix3& rotation)
{
	memory->write<math::matrix3>(this->address + Offsets::Camera::Rotation, rotation);
}