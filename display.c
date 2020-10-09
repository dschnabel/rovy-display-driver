#include "display.h"

#include "bcm_host.h"
#include "ilclient.h"

static TUNNEL_T dd_tunnel[4];
static COMPONENT_T *dd_video_decode = NULL, *dd_video_scheduler = NULL, *dd_video_render = NULL, *dd_clock = NULL;
static COMPONENT_T *dd_list[5];
static ILCLIENT_T *dd_client;

static int _dd_port_settings_changed(unsigned int data_len) {
    static int port_settings_changed = 0;

    if (port_settings_changed != 0) {
        return 0;
    }

    if ((data_len > 0 && ilclient_remove_event(dd_video_decode, OMX_EventPortSettingsChanged, 131, 0, 0, 1) == 0) ||
            (data_len == 0 && ilclient_wait_for_event(dd_video_decode, OMX_EventPortSettingsChanged, 131, 0, 0, 1,
                    ILCLIENT_EVENT_ERROR | ILCLIENT_PARAMETER_CHANGED, 10000) == 0)) {

        port_settings_changed = 1;

        if (ilclient_setup_tunnel(dd_tunnel, 0, 0) != 0) {
            return -1;
        }

        ilclient_change_component_state(dd_video_scheduler, OMX_StateExecuting);

        // now setup tunnel to video_render
        if (ilclient_setup_tunnel(dd_tunnel+1, 0, 1000) != 0) {
            return -1;
        }

        ilclient_change_component_state(dd_video_render, OMX_StateExecuting);
    }

    return 0;
}

int dd_init() {
    bcm_host_init();

    memset(dd_list, 0, sizeof(dd_list));
    memset(dd_tunnel, 0, sizeof(dd_tunnel));

    if ((dd_client = ilclient_init()) == NULL) {
       return -1;
    }

    if (OMX_Init() != OMX_ErrorNone) {
       ilclient_destroy(dd_client);
       return -1;
    }

    // create video_decode
    if (ilclient_create_component(dd_client, &dd_video_decode, "video_decode",
            (ILCLIENT_CREATE_FLAGS_T)(ILCLIENT_DISABLE_ALL_PORTS | ILCLIENT_ENABLE_INPUT_BUFFERS)) != 0) {
        return -1;
    }
    dd_list[0] = dd_video_decode;

    // create video_render
    if (ilclient_create_component(dd_client, &dd_video_render, "video_render", ILCLIENT_DISABLE_ALL_PORTS) != 0) {
        return -1;
    }
    dd_list[1] = dd_video_render;

    OMX_CONFIG_ROTATIONTYPE rotation;
    rotation.nSize = sizeof(OMX_CONFIG_ROTATIONTYPE);
    rotation.nVersion.nVersion = OMX_VERSION;
    rotation.nPortIndex = 90;
    rotation.nRotation = 180;
    OMX_ERRORTYPE eError = OMX_SetConfig(ILC_GET_HANDLE(dd_video_render), OMX_IndexConfigCommonRotate, &rotation);

    // create clock
    if (ilclient_create_component(dd_client, &dd_clock, "clock", ILCLIENT_DISABLE_ALL_PORTS) != 0) {
        return -1;
    }
    dd_list[2] = dd_clock;

    OMX_TIME_CONFIG_CLOCKSTATETYPE cstate;
    memset(&cstate, 0, sizeof(cstate));
    cstate.nSize = sizeof(cstate);
    cstate.nVersion.nVersion = OMX_VERSION;
    cstate.eState = OMX_TIME_ClockStateWaitingForStartTime;
    cstate.nWaitMask = 1;
    if (dd_clock != NULL
            && OMX_SetParameter(ILC_GET_HANDLE(dd_clock), OMX_IndexConfigTimeClockState, &cstate) != OMX_ErrorNone) {
        return -1;
    }

    // create video_scheduler
    if (ilclient_create_component(dd_client, &dd_video_scheduler, "video_scheduler", ILCLIENT_DISABLE_ALL_PORTS) != 0) {
        return -1;
    }
    dd_list[3] = dd_video_scheduler;

    set_tunnel(dd_tunnel, dd_video_decode, 131, dd_video_scheduler, 10);
    set_tunnel(dd_tunnel+1, dd_video_scheduler, 11, dd_video_render, 90);
    set_tunnel(dd_tunnel+2, dd_clock, 80, dd_video_scheduler, 12);

    // setup clock tunnel first
    if (ilclient_setup_tunnel(dd_tunnel+2, 0, 0) != 0) {
        return -1;
    }

    ilclient_change_component_state(dd_clock, OMX_StateExecuting);
    ilclient_change_component_state(dd_video_decode, OMX_StateIdle);

    OMX_VIDEO_PARAM_PORTFORMATTYPE format;
    memset(&format, 0, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
    format.nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
    format.nVersion.nVersion = OMX_VERSION;
    format.nPortIndex = 130;
    format.eCompressionFormat = OMX_VIDEO_CodingAVC;

    if (OMX_SetParameter(ILC_GET_HANDLE(dd_video_decode), OMX_IndexParamVideoPortFormat, &format) != OMX_ErrorNone) {
        return -1;
    }

    if (ilclient_enable_port_buffers(dd_video_decode, 130, NULL, NULL, NULL) != 0) {
        return -1;
    }

    ilclient_change_component_state(dd_video_decode, OMX_StateExecuting);

    return 0;
}

void dd_destroy() {
    ilclient_flush_tunnels(dd_tunnel, 0);

    ilclient_disable_tunnel(dd_tunnel);
    ilclient_disable_tunnel(dd_tunnel+1);
    ilclient_disable_tunnel(dd_tunnel+2);
    ilclient_disable_port_buffers(dd_video_decode, 130, NULL, NULL, NULL);
    ilclient_teardown_tunnels(dd_tunnel);

    ilclient_state_transition(dd_list, OMX_StateIdle);
    ilclient_state_transition(dd_list, OMX_StateLoaded);

    ilclient_cleanup_components(dd_list);

    OMX_Deinit();

    ilclient_destroy(dd_client);
}

int dd_play_video(unsigned char *buffer, uint len) {
    OMX_BUFFERHEADERTYPE *buf;
    int first_packet = 1;

    while ((buf = ilclient_get_input_buffer(dd_video_decode, 130, 1)) != NULL) {
        memset(buf->pBuffer, 0, buf->nAllocLen);

        uint size = buf->nAllocLen < len ? buf->nAllocLen : len;
        memcpy(buf->pBuffer, buffer, size);
        len -= size;

        _dd_port_settings_changed(size);

        if (!size) break;

        buf->nFilledLen = size;
        buf->nOffset = 0;
        if (first_packet) {
            buf->nFlags = OMX_BUFFERFLAG_STARTTIME;
            first_packet = 0;
        } else {
            buf->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN;
        }

        if (OMX_EmptyThisBuffer(ILC_GET_HANDLE(dd_video_decode), buf) != OMX_ErrorNone) {
            return -1;
        }
    }

    if (!buf) {
        return -1;
    }

    buf->nFilledLen = 0;
    buf->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN | OMX_BUFFERFLAG_EOS;

    if (OMX_EmptyThisBuffer(ILC_GET_HANDLE(dd_video_decode), buf) != OMX_ErrorNone) {
        return -1;
    }

    ilclient_wait_for_event(dd_video_render, OMX_EventBufferFlag,
            90, 0, OMX_BUFFERFLAG_EOS, 0, ILCLIENT_BUFFER_FLAG_EOS, -1);

    return 0;
}
