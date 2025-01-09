P4_16_PROGRAMS = [
        "bri_handle",
        "bri_with_pdfixed_thrift",
        "tna_32q_2pipe",
        "tna_action_profile",
        "tna_action_selector",
        "tna_bridged_md",
        "tna_checksum",
        "tna_counter",
        "tna_custom_hash",
        "tna_digest",
        "tna_dkm",
        "tna_dyn_hashing",
        "tna_exact_match",
        "tna_field_slice",
        "tna_idletimeout",
        "tna_lpm_match",
        "tna_meter_bytecount_adjust",
        "tna_meter_lpf_wred",
        "tna_mirror",
        "tna_multicast",
        "tna_operations",
        "tna_pktgen",
        "tna_port_metadata",
        "tna_port_metadata_extern",
        "tna_ports",
        "tna_proxy_hash",
        "tna_pvs",
        "tna_random",
        "tna_range_match",
        "tna_register",
        "tna_resubmit",
        "tna_snapshot",
        "tna_symmetric_hash",
        "tna_ternary_match",
        "tna_timestamp",
        "tna_32q_multiprogram"
]

P4_14_PROGRAMS = [
        "alpm_test",
        "basic_switching",
        "chksum",
        "default_entry",
        "deparse_zero",
        "dkm",
        "fast_reconfig",
        "ha",
        "hash_driven",
        "iterator",
        "meters",
        "mirror_test",
        "multicast_test",
        "parser_error",
        "parser_intr_md",
        "pcie_pkt_test",
        "perf_test",
        "perf_test_alpm",
        "pvs",
        "resubmit",
        "smoke_large_tbls"
]

INTERNAL_P4_16_PROGRAMS = [
    "misc1",
    "ipv4_checksum",
    "tna_stful",
    "snapshot",
    "bfrt_tm",
    "bfrt_tm_queue",
    "bfrt_dev",
    "digest_test",
    "tna_hash",
    "selector_resize",
    "snapshot_all_stages",
    "snapshot_all_stages_egress",
    "snapshot_phv"
]

INTERNAL_P4_14_PROGRAMS = [
    "range",
    "simple_l3_checksum_single_end",
    "basic_ipv4",
    "entry_read_from_hw",
    "exm_direct_1",
    "exm_indirect_1",
    "exm_smoke_test",
    "incremental_checksum",
    "mod_field_conditionally",
    "mod_with_shift",
    "opcode_test",
    "opcode_test_saturating",
    "opcode_test_signed",
    "opcode_test_signed_and_saturating",
    "simple_l3_checksum_branched_end",
    "simple_l3_checksum_taken_default_ingress",
    "simple_l3_mirror",
    "hash_test",
    "dyn_hash" ,
    "tcam_use_valid",
    "pctr"
]

TOFINO_2_PROGRAMS = [
    "t2na_fifo", // internal p4-16
    "hwlrn", // internal p4-16
    "t2na_counter_true_egress_accounting",
    "t2na_static_entry", // internal p4-16
    "alpm_test" //p4-14 tests
]

x1AsanTestList = [ "-s \" all ^switch_acl2 ^switch_hash \"" ]

x1SaiTestList = [ "-s \"saitest saifdb saivlan saiport sailag sairif ^tunnel saihostif saibridgeport saimirror ^mirror-meters saivrf saischeduler saischedulergroup saihash saiwred ^wred-traffic saipolicer ^hw ^port-meters sainexthop ^tunnel sairoute saidebugcounters saiqosmap saiswitch ^tunnel ^hw\" -t ptf/saiv2" ]

def executeTestProfile(String test_target) {

    // Update git to most recent version to enable parallel git submodule update
    sh '''
    apt-get update && apt-get install -y software-properties-common
    add-apt-repository -y ppa:git-core/ppa
    apt-get update && apt-get install -y psmisc git rsync
    rm -rf /p4factory/install/lib/python*/site-packages/bf_pktpy
    cp -rf bf_pktpy /p4factory/install/lib/python3.8/site-packages/
    mkdir -p xml_out
    '''

    switch(test_target) {

        case "p4-14-programs":
            runP4Tests(test_target, P4_14_PROGRAMS)
            break
        case "p4-16-programs":
            runP4Tests(test_target, P4_16_PROGRAMS)
            break
        case "internal-p4-14-programs":
            runP4Tests(test_target, INTERNAL_P4_14_PROGRAMS)
            break
        case "internal-p4-16-programs":
            runP4Tests(INTERNAL_P4_16_PROGRAMS.toString().replaceAll("[\\[\\]]", "").replaceAll(",", " "), INTERNAL_P4_16_PROGRAMS)
            break
        case "tofino2-p4-programs":
            runP4Tests(TOFINO_2_PROGRAMS.toString().replaceAll("[\\[\\]]", "").replaceAll(",", " "), TOFINO_2_PROGRAMS, "tofino2")
            break
        case "x1_tofino":
            runSwitchTests("x1_tofino", x1AsanTestList)
            break
        default:
            break
    }

}

def runSwitchTests(String make_target, List<String> testList, build_sai = false) {

    buildSwitch(make_target, build_sai)
    test_script = (build_sai == true) ? "test-bf-switch-sai.sh" : "test-bf-switch.sh"

    def arch = ""
    if (make_target.contains("tofino2")) {
        arch="--arch tofino2"
    }

    try {
        for (test in testList)
            // Dummy W/A for unsetting WORKSPACE variable for ptf test scripts.
            sh '''
            ### Running '''+test+ ''' test
            export WORKSPACE=/bf-switch
            /sandals/sde-test/docker/'''+test_script+'''  --gen-xml-output '''+test+''' '''+arch+''' --python3
            '''
    } finally {
        sh '''
        rsync -a /p4factory/submodules/ptf-utils/xml_out/* $(pwd)/xml_out
        '''
    }
}

def runP4Tests(String make_target, List<String> testList, arch = "tofino") {
    buildP4Tests(make_target, arch)

    for (test in testList)
        try {
                sh '''
                ### Running '''+test+ ''' test
                cd ci_workspace
                export SDE=`pwd`
                export WORKSPACE=`pwd`
                export SDE_INSTALL=$SDE/install

                default_negative_timeout="0.05"
                default_timeout="3"
                $WORKSPACE/tools/run_tofino_model.sh -q -p '''+test+''' --arch='''+arch+''' >tofino-model.log 2>tofino-model_err.log &
                $WORKSPACE/tools/run_switchd.sh -p '''+test+''' --arch='''+arch+''' 2>&1 > /dev/null &>tofino-driver.log &
                $WORKSPACE/tools/run_p4_tests.sh --gen-xml-output -p '''+test+''' --arch='''+arch+''' --default-negative-timeout $default_negative_timeout --default-timeout $default_timeout
                set +e
                killall -9 -r tofino-model
                killall -9 -r bf_switchd
                set -e
                '''
        } finally {
            sh '''
            rsync -a $WORKSPACE/ci_workspace/submodules/ptf-utils/xml_out/* $(pwd)/xml_out
            '''
        }

}

def makePktpyUnitTests() {
    sh '''
    python3 -m pip install setuptools wheel
    pip3 install scapy --ignore-installed

    cd $WORKSPACE
    pip3 install -r requirements-dev.txt
    make all

    pytest tests/ --profile
    python3 -m coverage run -m pytest tests/basic/ tests/core/library/utils/test_hexdump.py tests/core/library/specs/test_base_getlayer.py tests/core/library/specs/test_base_load_bytes.py tests/core/library/specs/test_base_remove_payload.py tests/core/library/specs/test_packet.py
    python3 -m coverage report --omit="*/tests*" | tail -c 5 > cov_result
    '''

}

def buildSwitch(String make_target, boolean build_sai = false) {

  sai_parameter = (build_sai == true) ? " -DSAI=on" : ""

  sh '''
  git clone git@github.com:intel-restricted/networking.switching.barefoot.bf-switch.git /bf-switch --single-branch --branch $TARGET_BRANCH -j 8
  cd /bf-switch
  git rev-parse HEAD
  git clean -ffdx
  git submodule sync --recursive
  git submodule update --init --recursive -j 8
  git submodule foreach --recursive 'git clean -ffdx'

  mkdir -p /build &&   cd /build
  cmake /bf-switch \
    -DPYTHON_EXECUTABLE=python3 \
    -DSTANDALONE=on \
    -DCMAKE_MODULE_PATH=/bf/cmake \
    -DCMAKE_INSTALL_PREFIX=/bf/install/'''+sai_parameter+'''
  make -j8 '''+make_target+'''
  make -j8 install
  cd -
  '''
}

// Possible make_target values are: p4-14-programs, p4-16-programs, internal-p4-14-programs, internal-p4-16-programs
def buildP4Tests(String make_target, architecture)
{
    if (architecture == "tofino") {
        CMAKE_ARCH = "-DTOFINO=ON -DTOFINO2=OFF"
    }
    else {
        CMAKE_ARCH = "-DTOFINO=OFF -DTOFINO2=ON"
    }

    sh '''
    ### Build P4 tests: '''+make_target+'''
    git clone --single-branch --branch $TARGET_BRANCH git@github.com:intel-restricted/networking.switching.barefoot.p4factory ci_workspace -j 8 || true
    cd ci_workspace
    git status
    git fetch
    git reset --hard origin/$TARGET_BRANCH
    git -c submodule."submodules/bf-p4c-compilers".update=none -c submodule."submodules/model".update=none submodule update --recursive --init -j 8
    submodules/sandals/jenkins/veth_setup.sh 128 &> /dev/null
    submodules/sandals/jenkins/disable_ipv6.sh

    mkdir -p build && cd build
    cmake .. '''+CMAKE_ARCH+'''
    make -j8 '''+make_target+'''
    make -j8 install
    rm -rf $WORKSPACE/ci_workspace/install/lib/python*/site-packages/bf_pktpy
    cp -rf $WORKSPACE/bf_pktpy $WORKSPACE/ci_workspace/install/lib/python3.8/site-packages/
    '''
}

def coverageComment=''

node('master') {
 stage('Determine Target Branch') {
  // The 'CHANGE_TARGET' env var is set by Jenkins and contains the value of the
  // branch the PR will be merged into if everything works well. We use this
  // variable to decide which p4factory docker image to use
  if (env.CHANGE_TARGET == null) {
     env.CHANGE_TARGET = "master"
  }

  target_branch = "${env.CHANGE_TARGET}"

 }
}

pipeline {
    agent { label 'mateusz' }
    environment {
        PKTPY = "True"
        TARGET_BRANCH = "${target_branch}"
    }
    options {
        timestamps()
        timeout(time: 180, unit: 'MINUTES')
    }
   stages {
       stage("Quick verification") {
           parallel {
                   stage("Code formattter (using black)") {
                    agent{
                        docker {
                            label 'mateusz'
                            image "cytopia/black"
                            args '--entrypoint='
                            reuseNode false
                            alwaysPull true
                        }
                    }
                    steps {
                        sh '''
                          echo "Black version"
                          black --version
                          echo "Check with Black"
                          black --check bf_pktpy/
                        '''
                    }
                }
                stage("Code linter (using flake8)") {
                    agent{
                        docker {
                            label 'mateusz'
                            image "artifacts-bxdsw.sc.intel.com:9444/p4factory:${target_branch}"
                            registryUrl 'https://artifacts-bxdsw.sc.intel.com:9444'
                            registryCredentialsId 'nexus-docker-creds'
                            args '--privileged --cap-add=ALL  --user=root -v /lib/modules:/lib/modules:ro -v /usr/src:/usr/src:ro'
                            reuseNode false
                            alwaysPull true
                        }
                    }
                    steps {
                        sh '''
                          echo "Install Flake8"
                          python3 -m pip install flake8==3.8.0
                        '''
                        sh '''
                          echo "Check with Flake8"
                          flake8 --ignore E501,W503,E203,F841 \
                          --exclude "__init__.py","tests","ci_workspace","build"
                        '''
                    }
                }
                stage("Build, install & run unit tests [bf-pktpy]") {
                    agent{
                        docker {
                            label 'mateusz'
                            image "artifacts-bxdsw.sc.intel.com:9444/p4factory:${target_branch}"
                            registryUrl 'https://artifacts-bxdsw.sc.intel.com:9444'
                            registryCredentialsId 'nexus-docker-creds'
                            args '--privileged --cap-add=ALL  --user=root -v /lib/modules:/lib/modules:ro -v /usr/src:/usr/src:ro'
                            reuseNode false
                            alwaysPull true
                        }
                    }
                    steps {
                        echo "Make bf-pktpy unit tests"
                        makePktpyUnitTests()
                        archiveArtifacts allowEmptyArchive: true, artifacts: 'prof/*', onlyIfSuccessful: false
                        script {
                         coverageComment = readFile(file: 'cov_result').trim()
                        }
                    }
                }
           }
       }
       stage("Display UT coverage"){
        agent{
            docker {
                label 'mateusz'
                image "artifacts-bxdsw.sc.intel.com:9444/p4factory:${target_branch}"
                registryUrl 'https://artifacts-bxdsw.sc.intel.com:9444'
                registryCredentialsId 'nexus-docker-creds'
                args '--privileged --cap-add=ALL  --user=root -v /lib/modules:/lib/modules:ro -v /usr/src:/usr/src:ro'
                reuseNode false
                alwaysPull true
            }
        }
        steps {
             script {
                    if (env.CHANGE_ID) {
                        pullRequest.comment("UT coverage: ${coverageComment}")
                    }
                }
             }
        }
        stage("Test bf-pktpy") {
            matrix {
                axes {
                    axis {
                        name "TEST_TARGET"
                        values "x1_tofino", "p4-14-programs", "p4-16-programs", "internal-p4-14-programs", "internal-p4-16-programs", "tofino2-p4-programs"  // "x1_tofino_SAI_legacy", "x2_tofino", "x2_hitless", "x2_tofino_sai", "x2_tofino_sai_legacy", "x1_fast_reconfig", "x1_hitless", "y1_tofino_2", "y1_tofino_2_sai", "y2_tofino_2", "z2_tofino"
                    }
                }
                stages {
                    stage("Testing") {
                        agent {
                                docker {
                                    label 'mateusz'
                                    image "artifacts-bxdsw.sc.intel.com:9444/p4factory:${target_branch}"
                                    registryUrl 'https://artifacts-bxdsw.sc.intel.com:9444'
                                    registryCredentialsId 'nexus-docker-creds'
                                    args '--privileged --cap-add=ALL  --user=root -v /usr/src:/usr/src:ro'
                                    reuseNode false
                                    alwaysPull true
                                }
                            }

                        steps {
                            executeTestProfile(TEST_TARGET)

                        }
                        post {
                            always {
                                sh ''' ls -lah '''
                                junit 'xml_out/**/*.xml'
                            }
                        }
                    }
                }

            }

        }
    }
    post {
        always {
            archiveArtifacts allowEmptyArchive: true, artifacts: 'ci_workspace/*.log', fingerprint: true, onlyIfSuccessful: false

        }
    }
}
