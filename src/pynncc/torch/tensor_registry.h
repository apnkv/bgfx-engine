#pragma once

#include <iostream>
#include <numeric>
#include <unordered_set>

#include <cpp_redis/cpp_redis>
#include <bgfx/bgfx.h>
#include <fmt/format.h>
#include <torch/torch.h>
#include <libshm/libshm.h>

#include <nncc/common/types.h>
#include <nncc/context/context.h>
#include <nncc/rendering/renderer.h>
#include <nncc/rendering/primitives.h>


struct TensorControl {
    bool controllable = true;
    nncc::string callback_name;
};


struct RetrieveKey {
    template<typename T>
    typename T::first_type operator()(T key_and_value) const {
        return key_and_value.first;
    }
};

namespace nncc {

bgfx::TextureFormat::Enum GetTextureFormatFromChannelsAndDtype(int64_t channels, const torch::Dtype& dtype);


class TensorWithPointer {
public:
    TensorWithPointer(
        const nncc::string& manager_handle,
        const nncc::string& filename,
        torch::Dtype dtype,
        const nncc::vector<int64_t>& dims
    ) {
        size_t total_bytes = (
            torch::elementSize(dtype)
                * std::accumulate(dims.begin(), dims.end(), 1, std::multiplies<size_t>())
        );

        data_ptr_ = THManagedMapAllocator::makeDataPtr(
            manager_handle.c_str(),
            filename.c_str(),
            at::ALLOCATOR_MAPPED_SHAREDMEM,
            total_bytes
        );
        tensor_ = torch::from_blob(data_ptr_.get(),
                                   at::IntArrayRef(dims.data(), dims.size()),
                                   torch::TensorOptions().dtype(dtype));
    }

    const torch::Tensor& operator*() const {
        return tensor_;
    }

    torch::Tensor& operator*() {
        return const_cast<torch::Tensor&>(const_cast<const TensorWithPointer*>(this)->operator*());
    }

private:
    at::DataPtr data_ptr_;
    torch::Tensor tensor_;
};


struct Name {
    Name() = default;

    Name(const nncc::string& _value) : value(_value) {}

    nncc::string value;
};


struct TensorControlEvent {
    entt::entity tensor_entity;
    std::optional<nncc::string> callback_name;
};

struct SharedTensorEvent {
    nncc::string name;
    nncc::string manager_handle;
    nncc::string filename;
    torch::Dtype dtype;
    nncc::vector<int64_t> dims;
};


class TensorRegistry {
public:
    TensorRegistry() = default;

    void Init(entt::dispatcher* dispatcher) {
        dispatcher->sink<SharedTensorEvent>().connect<&TensorRegistry::OnSharedTensorUpdate>(*this);
        dispatcher->sink<TensorControlEvent>().connect<&TensorRegistry::OnSharedTensorControl>(*this);
        redis_.connect();
    }

    void OnTensorWithPointerDestroy(entt::registry& registry, entt::entity entity) {
        tensors_.erase(names_[entity]);
        names_.erase(entity);
        if (drawable_.contains(entity)) {
            drawable_.erase(entity);
        }
    }

    void OnSharedTensorUpdate(const SharedTensorEvent& event);

    void OnSharedTensorControl(const TensorControlEvent& event);

    void Update() {
        redis_.commit();
    }

    entt::entity Get(const nncc::string& name) {
        return tensors_.at(name);
    }

    bool Contains(const nncc::string& name) {
        return tensors_.contains(name);
    }

    void Clear();

private:
    std::unordered_map<nncc::string, entt::entity> tensors_;
    std::unordered_set<entt::entity> drawable_;
    std::unordered_map<entt::entity, nncc::string> names_;

    cpp_redis::client redis_;
};

}
