source ../build_scripts/version.sh || exit 1

source ../build_scripts/add_icon.sh || exit 1
source ../build_scripts/build_all.sh || exit 1
source ../build_scripts/build_bundle.sh || exit 1
source ../build_scripts/build_dmg.sh || exit 1
source ../build_scripts/build_documentation.sh || exit 1
source ../build_scripts/build_main_entry_point.sh || exit 1
source ../build_scripts/build_macos_entry_point.sh || exit 1
source ../build_scripts/build_monitor.sh || exit 1
source ../build_scripts/build_package.sh || exit 1
source ../build_scripts/build_wheel.sh || exit 1
source ../build_scripts/checkout_arrow.sh || exit 1
source ../build_scripts/checkout_engine.sh || exit 1
source ../build_scripts/checkout_postgres.sh || exit 1
source ../build_scripts/checkout_mariadb.sh || exit 1
source ../build_scripts/checkout_poco.sh || exit 1
source ../build_scripts/checkout_rangev3.sh || exit 1
source ../build_scripts/checkout_xgboost.sh || exit 1
source ../build_scripts/compile_arrow.sh || exit 1
source ../build_scripts/compile_engine.sh || exit 1
source ../build_scripts/compile_mariadb.sh || exit 1
source ../build_scripts/compile_poco.sh || exit 1
source ../build_scripts/compile_postgres.sh || exit 1
source ../build_scripts/compile_unixodbc.sh || exit 1
source ../build_scripts/compile_xgboost.sh || exit 1
source ../build_scripts/copy_dlls.sh || exit 1
source ../build_scripts/copy_dylibs.sh || exit 1
source ../build_scripts/copy_libs.sh || exit 1
source ../build_scripts/copy_python.sh || exit 1
source ../build_scripts/copy_templates.sh || exit 1
source ../build_scripts/download_engine.sh || exit 1
source ../build_scripts/download_unixodbc.sh || exit 1
source ../build_scripts/exec_getml.sh || exit 1
source ../build_scripts/git_branch_develop_update.sh || exit 1
source ../build_scripts/git_branch_update.sh || exit 1
source ../build_scripts/git_handle_branch.sh || exit 1
source ../build_scripts/handle_dylibs.sh || exit 1
source ../build_scripts/init_docker.sh || exit 1
source ../build_scripts/init_repositories.sh || exit 1
source ../build_scripts/install_dependencies.sh || exit 1
source ../build_scripts/install_python.sh || exit 1
source ../build_scripts/install_unixodbc.sh || exit 1
source ../build_scripts/macos_start_docker.sh || exit 1
source ../build_scripts/make_checksum.sh || exit 1
source ../build_scripts/release.sh || exit 1
source ../build_scripts/relink_dylibs_in_executable.sh || exit 1
source ../build_scripts/relink_dylibs_in_odbc.sh || exit 1
source ../build_scripts/relink_dylibs_in_ssl.sh || exit 1
source ../build_scripts/relink_dylibs_in_xgboost.sh || exit 1
source ../build_scripts/serve_frontend.sh || exit 1
source ../build_scripts/zip_docker.sh || exit 1
