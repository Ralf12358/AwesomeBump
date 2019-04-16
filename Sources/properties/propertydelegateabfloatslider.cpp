#include "Delegates/PropertyDelegateFactory.h"
#include "Delegates/Utils/PropertyDelegateSliderBox.h"
#include "Core/PropertyFloat.h"
#include "Core/PropertyInt.h"

void regABSliderDelegates()
{
    QtnPropertyDelegateFactory::staticInstance()
        .registerDelegateDefault(&QtnPropertyFloatBase::staticMetaObject
                                , &qtnCreateDelegate<QtnPropertyDelegateSlideBoxTyped<QtnPropertyFloatBase>, QtnPropertyFloatBase>
                                , "SliderBox");

    QtnPropertyDelegateFactory::staticInstance()
        .registerDelegateDefault(&QtnPropertyIntBase::staticMetaObject
                                , &qtnCreateDelegate<QtnPropertyDelegateSlideBoxTyped<QtnPropertyIntBase>, QtnPropertyIntBase>
                                , "SliderBox");
}
