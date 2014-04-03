/*
* UsbDk filter/redirector driver
*
* Copyright (c) 2013  Red Hat, Inc.
*
* Authors:
*  Dmitry Fleytman  <dfleytma@redhat.com>
*
*/

//
// Define the tracing flags.
//
// Tracing GUID - 88e1661f-48b6-410f-b096-ba84e9f0656f
//

#define WPP_CONTROL_GUIDS                                              \
    WPP_DEFINE_CONTROL_GUID(                                           \
        UsbDkTraceGuid, (88e1661f,48b6,410f,b096,ba84e9f0656f), \
                                                                            \
        WPP_DEFINE_BIT(MYDRIVER_ALL_INFO)                              \
        WPP_DEFINE_BIT(TRACE_DRIVER)                                   \
        WPP_DEFINE_BIT(TRACE_DEVICE)                                   \
        WPP_DEFINE_BIT(TRACE_FILTER)                                   \
        WPP_DEFINE_BIT(TRACE_DEVACCESS)                                \
        WPP_DEFINE_BIT(TRACE_REGTEXT)                                  \
        WPP_DEFINE_BIT(TRACE_CONTROLDEVICE)                            \
        WPP_DEFINE_BIT(TRACE_FILTERDEVICE)                             \
        WPP_DEFINE_BIT(TRACE_WDFDEVICE)                                \
        WPP_DEFINE_BIT(TRACE_REDIRECTOR)                               \
        )

#define WPP_FLAG_LEVEL_LOGGER(flag, level)                                  \
    WPP_LEVEL_LOGGER(flag)

#define WPP_FLAG_LEVEL_ENABLED(flag, level)                                 \
    (WPP_LEVEL_ENABLED(flag) &&                                             \
     WPP_CONTROL(WPP_BIT_ ## flag).Level >= level)

#define WPP_LEVEL_FLAGS_LOGGER(lvl,flags) \
           WPP_LEVEL_LOGGER(flags)

#define WPP_LEVEL_FLAGS_ENABLED(lvl, flags) \
           (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level >= lvl)

//
// This comment block is scanned by the trace preprocessor to define our
// Trace function.
//
// begin_wpp config
// FUNC Trace{FLAG=MYDRIVER_ALL_INFO}(LEVEL, MSG, ...);
// FUNC TraceEvents(LEVEL, FLAGS, MSG, ...);
// CUSTOM_TYPE(devprop, ItemEnum(DEVICE_REGISTRY_PROPERTY));
// CUSTOM_TYPE(devid, ItemEnum(BUS_QUERY_ID_TYPE));
// end_wpp
//
