#define __HIP_PLATFORM_HCC__
#define _GNU_SOURCE
#include <dlfcn.h>

#include "amd-dbgapi.h"

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>

static amd_dbgapi_callbacks_t callbacks;
static amd_dbgapi_process_id_t self;

void __attribute__((constructor)) init();

// roctx header file
#include <roctx.h>
// roctracer extension API
#include <roctracer_ext.h>


#include <roctracer_hip.h>
#include <roctracer_hcc.h>
#include <roctracer_kfd.h>
#include <roctracer_roctx.h>

#include <unistd.h> 
#include <sys/syscall.h>   /* For SYS_xxx definitions */ 

// Macro to check ROC-tracer calls status
#define ROCTRACER_CALL(call)                                                                       \
  do {                                                                                             \
    int err = call;                                                                                \
    if (err != 0) {                                                                                \
      fprintf(stderr, "%s\n", roctracer_error_string());                                                    \
      abort();                                                                                     \
    }                                                                                              \
  } while (0)


amd_dbgapi_status_t self_process(amd_dbgapi_client_process_id_t cp,
		amd_dbgapi_os_pid_t *os_pid) {
	fprintf(stderr, "enter self_process\n");
	*os_pid = getpid();
	return AMD_DBGAPI_STATUS_SUCCESS;
}

amd_dbgapi_status_t enable_notify_shared_library_empty (
      amd_dbgapi_client_process_id_t client_process_id,
      const char *shared_library_name,
      amd_dbgapi_shared_library_id_t shared_library_id,
      amd_dbgapi_shared_library_state_t *shared_library_state) {
	fprintf(stderr, "enter enable_notify_shared_library_empty %s\n", shared_library_name);
	if (strcmp(shared_library_name, "libhsa-runtime64.so.1") == 0)
		*shared_library_state = AMD_DBGAPI_SHARED_LIBRARY_STATE_LOADED;
	else
		*shared_library_state = AMD_DBGAPI_SHARED_LIBRARY_STATE_UNLOADED;
	return AMD_DBGAPI_STATUS_SUCCESS;
}

amd_dbgapi_status_t disable_notify_shared_library_empty (
      amd_dbgapi_client_process_id_t client_process_id,
      amd_dbgapi_shared_library_id_t shared_library_id) {
	return AMD_DBGAPI_STATUS_SUCCESS;
}


amd_dbgapi_status_t get_symbol_address_empty (
      amd_dbgapi_client_process_id_t client_process_id,
      amd_dbgapi_shared_library_id_t shared_library_id,
      const char *symbol_name, amd_dbgapi_global_address_t *address) {
	*address = (amd_dbgapi_global_address_t) dlsym(RTLD_DEFAULT, symbol_name);
	return AMD_DBGAPI_STATUS_SUCCESS;
}

amd_dbgapi_status_t insert_breakpoint_empty (
      amd_dbgapi_client_process_id_t client_process_id,
      amd_dbgapi_shared_library_id_t shared_library_id,
      amd_dbgapi_global_address_t address,
      amd_dbgapi_breakpoint_id_t breakpoint_id) {
	return AMD_DBGAPI_STATUS_SUCCESS;
}

amd_dbgapi_status_t remove_breakpoint_empty (
      amd_dbgapi_client_process_id_t client_process_id,
      amd_dbgapi_breakpoint_id_t breakpoint_id) {
	return AMD_DBGAPI_STATUS_SUCCESS;
}

void log_message_empty (amd_dbgapi_log_level_t level, const char *message) {
	fprintf(stderr, "%s\n", message);
}

void check_status(amd_dbgapi_status_t ret, int lineNo) {
	if (ret == AMD_DBGAPI_STATUS_SUCCESS) return;
	if (ret == AMD_DBGAPI_STATUS_FATAL) {
		fprintf(stderr, "FATAL");
	} else if (ret == AMD_DBGAPI_STATUS_ERROR_NOT_INITIALIZED) {
		fprintf(stderr, "Not inited");
	} else if (ret == AMD_DBGAPI_STATUS_ERROR_INVALID_PROCESS_ID) {
		fprintf(stderr, "invalid pid");
	} else if (ret == AMD_DBGAPI_STATUS_ERROR_INVALID_ARGUMENT) {
		fprintf(stderr, "invalid argument");
	} else if (ret == AMD_DBGAPI_STATUS_ERROR_CLIENT_CALLBACK) {
		fprintf(stderr, "error client callback");
	} else if (ret == AMD_DBGAPI_STATUS_ERROR_INVALID_CODE_OBJECT_ID) {
		fprintf(stderr, "invalid code object id\n");
	} else if (ret == AMD_DBGAPI_STATUS_ERROR_INVALID_ARGUMENT_SIZE) {
		fprintf(stderr, "invalid argument size\n");
	} else {
		fprintf(stderr, "unknown ret");
	}
	fprintf(stderr, " at line %d\n", lineNo);
	exit(0);
}

void debug_init() {
	fprintf(stderr, "initialize debugger\n");
	callbacks.allocate_memory = malloc;
	callbacks.deallocate_memory = free;
	callbacks.get_os_pid = self_process;
	callbacks.enable_notify_shared_library = enable_notify_shared_library_empty;
	callbacks.disable_notify_shared_library = disable_notify_shared_library_empty;
	callbacks.get_symbol_address = get_symbol_address_empty;
	callbacks.insert_breakpoint = insert_breakpoint_empty;
	callbacks.remove_breakpoint = remove_breakpoint_empty;
	callbacks.log_message = log_message_empty;

	amd_dbgapi_status_t ret;
	ret = amd_dbgapi_initialize(&callbacks);
	check_status(ret, __LINE__);

	ret = amd_dbgapi_process_attach((amd_dbgapi_client_process_id_t)(&self), &self);
	check_status(ret, __LINE__);	
}

void dumpCodeObject(char* uri) {
	// URI example: memory://154772#offset=0x2aaed80ad1f0&size=41688
	if (strncmp(uri, "memory", strlen("memory")) == 0) {
		char* index = strstr(uri, "offset=") + strlen("offset=");
		char* endptr;
		unsigned long long offset = strtoull(index, &endptr, 16);
		
		index = strstr(uri, "size=") + strlen("size=");
		unsigned long int size = strtoul(index, &endptr, 10);
		fprintf(stderr, "\toffset %lx, size %lx\n", offset, size);

		int fd;
		fd = open("amd_gpu_binary", O_WRONLY | O_CREAT | O_TRUNC | O_EXCL, 0644);
		if (fd < 0) {
			fprintf(stderr, "Fail to create file\n");
			return;	
		}
		write(fd, (const void*)offset, size);
		close(fd);
	} else {
		fprintf(stderr, "Unhandled URI: %s\n", uri);
	}	
}

void queryCodeObject() { 	
	size_t code_object_count;	
	amd_dbgapi_code_object_id_t *code_objects_id;
	amd_dbgapi_status_t ret = amd_dbgapi_code_object_list(self, &code_object_count, &code_objects_id, NULL);
	check_status(ret, __LINE__);
	fprintf(stderr, "code object count %u\n", code_object_count);
	for (size_t i = 0; i < code_object_count; ++i) {
		char* uri;
		ret = amd_dbgapi_code_object_get_info(self, 
				code_objects_id[i],
				AMD_DBGAPI_CODE_OBJECT_INFO_URI_NAME,
				sizeof(char*),
				(void*)(&uri));
		check_status(ret, __LINE__);
		fprintf(stderr, "uri %s\n", uri);
		dumpCodeObject(uri);
	}
}

void activity_callback(const char* begin, const char* end, void* arg) {
}

static int counter = 0;

void api_callback(
    uint32_t domain,
    uint32_t cid,
    const void* callback_data,
    void* arg)
{
	if (domain != ACTIVITY_DOMAIN_HIP_API) return;
	if (cid != HIP_API_ID_hipLaunchKernel) return;
	if (counter == 0) {
		debug_init();
		queryCodeObject();
	}
	counter ++;
}

void init() {
	setenv("HIP_ENABLE_DEFERRED_LOADING", "0", 1);
	roctracer_set_properties(ACTIVITY_DOMAIN_HIP_API, NULL);
      	// Allocating tracing pool
	roctracer_properties_t properties;
	memset(&properties, 0, sizeof(roctracer_properties_t));
      	properties.buffer_size = 0x1000;
      	properties.buffer_callback_fun = activity_callback;
      	ROCTRACER_CALL(roctracer_open_pool(&properties));
      
	// Enable HIP API callbacks
	ROCTRACER_CALL(roctracer_enable_domain_callback(ACTIVITY_DOMAIN_HIP_API, api_callback, NULL));
	// Enable HIP activity tracing
	ROCTRACER_CALL(roctracer_enable_domain_activity(ACTIVITY_DOMAIN_HIP_API));
 	ROCTRACER_CALL(roctracer_enable_domain_activity(ACTIVITY_DOMAIN_HCC_OPS));
	ROCTRACER_CALL(roctracer_enable_domain_callback(ACTIVITY_DOMAIN_KFD_API, api_callback, NULL));
	ROCTRACER_CALL(roctracer_enable_domain_callback(ACTIVITY_DOMAIN_ROCTX, api_callback, NULL));

}
