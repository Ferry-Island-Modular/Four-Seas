#include "src/params.h"
#include "stmlib/stmlib.h"

namespace fourseas
{
void Params::Init(size_t size)
{
    size_ = size;
}

void Params::Update(Params::Values vals)
{
    frequency_interpolator_.Init(&values_.frequency, vals.frequency, size_);

    x_interpolator_.Init(&values_.x, vals.x, size_);
    y_interpolator_.Init(&values_.y, vals.y, size_);
    z_interpolator_.Init(&values_.z, vals.z, size_);

    osc_mod_interpolator_.Init(
        &values_.osc_mod_amount, vals.osc_mod_amount, size_);
}

Params::Values Params::Fetch()
{
    values_.frequency = frequency_interpolator_.Next();

    values_.x = x_interpolator_.Next();
    values_.y = y_interpolator_.Next();
    values_.z = z_interpolator_.Next();

    // TODO: this could be optimized since osc mod is N/A for derived outs
    values_.osc_mod_amount = osc_mod_interpolator_.Next();

    return values_;
}

} // namespace fourseas