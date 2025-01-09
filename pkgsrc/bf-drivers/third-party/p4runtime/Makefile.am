ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4

if IS_P4RT_ENABLED
# If PI is used (instead of BFRT), then there is no need to build the Protobuf
# libraries as they are already installed as part of the PI repository.
if !IS_PI_ENABLED

PROTO_DIR = $(abs_srcdir)/p4runtime/proto

protos = \
$(PROTO_DIR)/p4/v1/p4data.proto \
$(PROTO_DIR)/p4/v1/p4runtime.proto \
$(PROTO_DIR)/p4/config/v1/p4info.proto \
$(PROTO_DIR)/p4/config/v1/p4types.proto

proto_cpp_sources = \
cpp_out/p4/v1/p4data.pb.cc \
cpp_out/p4/v1/p4data.pb.h \
cpp_out/p4/v1/p4runtime.pb.cc \
cpp_out/p4/v1/p4runtime.pb.h \
cpp_out/p4/config/v1/p4info.pb.cc \
cpp_out/p4/config/v1/p4info.pb.h \
cpp_out/p4/config/v1/p4types.pb.cc \
cpp_out/p4/config/v1/p4types.pb.h

proto_grpc_sources = \
cpp_out/p4/v1/p4data.grpc.pb.cc \
cpp_out/p4/v1/p4data.grpc.pb.h \
cpp_out/p4/v1/p4runtime.grpc.pb.cc \
cpp_out/p4/v1/p4runtime.grpc.pb.h \
cpp_out/p4/config/v1/p4info.grpc.pb.cc \
cpp_out/p4/config/v1/p4info.grpc.pb.h \
cpp_out/p4/config/v1/p4types.grpc.pb.cc \
cpp_out/p4/config/v1/p4types.grpc.pb.h

includep4dir = $(includedir)/p4/v1/
nodist_includep4_HEADERS = \
cpp_out/p4/v1/p4data.pb.h \
cpp_out/p4/v1/p4data.grpc.pb.h \
cpp_out/p4/v1/p4runtime.pb.h \
cpp_out/p4/v1/p4runtime.grpc.pb.h

includep4configdir = $(includedir)/p4/config/v1/
nodist_includep4config_HEADERS = \
cpp_out/p4/config/v1/p4info.pb.h \
cpp_out/p4/config/v1/p4info.grpc.pb.h \
cpp_out/p4/config/v1/p4types.pb.h \
cpp_out/p4/config/v1/p4types.grpc.pb.h

BUILT_SOURCES = $(proto_cpp_sources) $(proto_grpc_sources)

p4pydir = $(pythondir)/p4
nodist_p4py_PYTHON = \
py_out/p4/__init__.py

p4v1pydir = $(pythondir)/p4/v1
nodist_p4v1py_PYTHON = \
py_out/p4/v1/p4data_pb2.py \
py_out/p4/v1/p4data_pb2_grpc.py \
py_out/p4/v1/p4runtime_pb2.py \
py_out/p4/v1/p4runtime_pb2_grpc.py \
py_out/p4/v1/__init__.py

p4configpydir = $(pythondir)/p4/config
nodist_p4configpy_PYTHON = \
py_out/p4/config/__init__.py

p4configv1pydir = $(pythondir)/p4/config/v1
nodist_p4configv1py_PYTHON = \
py_out/p4/config/v1/p4info_pb2.py \
py_out/p4/config/v1/p4info_pb2_grpc.py \
py_out/p4/config/v1/p4types_pb2.py \
py_out/p4/config/v1/p4types_pb2_grpc.py \
py_out/p4/config/v1/__init__.py

BUILT_SOURCES += \
$(nodist_p4py_PYTHON) \
$(nodist_p4v1py_PYTHON) \
$(nodist_p4configpy_PYTHON) \
$(nodist_p4configv1py_PYTHON)

PROTOFLAGS = -I$(PROTO_DIR) -I$(abs_top_srcdir)/third-party

# See http://www.gnu.org/software/automake/manual/html_node/Multiple-Outputs.html
proto_files.ts: $(protos)
	@rm -f proto_files.tmp
	@touch proto_files.tmp
	@mkdir -p $(builddir)/cpp_out
	@mkdir -p $(builddir)/py_out
	$(PROTOC) $^ --cpp_out $(builddir)/cpp_out \
	    --grpc_out $(builddir)/cpp_out \
            --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN) $(PROTOFLAGS)
	$(PROTOC) $^ --python_out $(builddir)/py_out \
            --grpc_out $(builddir)/py_out \
	    --plugin=protoc-gen-grpc=$(GRPC_PY_PLUGIN) $(PROTOFLAGS)
	@touch $(builddir)/py_out/p4/__init__.py $(builddir)/py_out/p4/v1/__init__.py
	@touch $(builddir)/py_out/p4/config/__init__.py $(builddir)/py_out/p4/config/v1/__init__.py
	@mv -f proto_files.tmp $@

$(BUILT_SOURCES): proto_files.ts
## Recover from the removal of $@
	@if test -f $@; then :; else \
	  trap 'rm -rf proto_files.lock proto_files.ts' 1 2 13 15; \
## mkdir is a portable test-and-set
	if mkdir proto_files.lock 2>/dev/null; then \
## This code is being executed by the first process.
	  rm -f proto_files.ts; \
	  $(MAKE) $(AM_MAKEFLAGS) proto_files.ts; \
	  result=$$?; rm -rf proto_files.lock; exit $$result; \
	else \
## This code is being executed by the follower processes.
## Wait until the first process is done.
	  while test -d proto_files.lock; do sleep 1; done; \
## Succeed if and only if the first process succeeded.
	    test -f proto_files.ts; \
	  fi; \
	fi

noinst_LTLIBRARIES = libbf_p4rt_protobuf.la libbf_p4rt_grpc.la

nodist_libbf_p4rt_protobuf_la_SOURCES = $(proto_cpp_sources)
nodist_libbf_p4rt_grpc_la_SOURCES = $(proto_grpc_sources)

libbf_p4rt_protobuf_la_LIBADD = $(PROTOBUF_LIBS)
libbf_p4rt_grpc_la_LIBADD = $(PROTOBUF_LIBS) $(GRPC_LIBS)

# include generated headers for P4RT and google protos
libbf_p4rt_protobuf_la_CPPFLAGS = -Icpp_out -I../cpp_out
libbf_p4rt_grpc_la_CPPFLAGS = -Icpp_out -I../cpp_out

CLEANFILES = $(BUILT_SOURCES) proto_files.ts

endif  # !IS_PI_ENABLED
endif  # IS_P4RT_ENABLED
