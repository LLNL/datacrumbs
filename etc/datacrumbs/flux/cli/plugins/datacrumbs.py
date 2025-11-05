##############################################################
# Copyright 2025 Lawrence Livermore National Security, LLC
# (c.f. AUTHORS, NOTICE.LLNS, COPYING)
#
# This file is part of the Flux resource manager framework.
# For details, see https://github.com/flux-framework.
#
# SPDX-License-Identifier: LGPL-3.0
##############################################################

from flux.cli.plugin import CLIPlugin
import os


class DatacrumbsPlugin(CLIPlugin):
    """
    Set the Datacrumbs option.
    """

    def __init__(self, prog, prefix="datacrumbs"):
        super().__init__(prog, prefix=prefix)
        self.add_option(
            "--enable",
            action="store_true",
            default=False,
            help="Enable datacrumbs"
        )
        self.add_option(
            "--composite",
            type=str,
            default="",
            help="Set composite name for datacrumbs"
        )
        
    def validate(self, jobspec):
        try:
            enable = jobspec.attributes["datacrumbs"]["enable"].lower() == "yes"
            if enable:
                composite_name = jobspec.attributes["datacrumbs"].get("composite", "")
                # Validate composite name if provided
                if composite_name != "":
                    if not composite_name.replace('_', '').isalpha():
                        raise ValueError("--composite can only contain alphabetic characters and underscores")
                    # TODO: Uncomment and adjust the following lines to validate the existence of the composite file
                    # user = jobspec.attributes["system"]["environment"].get("USER", "")
                    # if user == "":
                    #     raise ValueError("USER environment variable is required for datacrumbs composite validation")
                    # file = f"@CMAKE_INSTALL_PREFIX@/@CMAKE_INSTALL_LIBEXEC_DIR@/@PROJECT_NAME@/@CMAKE_INSTALL_SBINDIR@/{user}/datacrumbs_{composite_name}"
                    # if not os.path.exists(file):
                    #     raise ValueError(f"Datacrumbs composite: {composite_name} does not exist")
        except KeyError:
            enable = False
    
    def modify_jobspec(self, args, jobspec):
        if args.enable:
            jobspec.setattr("datacrumbs.enable", "yes")
            if args.composite != "":
                jobspec.setattr("datacrumbs.composite", args.composite)
        else:
            jobspec.setattr("datacrumbs.enable", "no")
            jobspec.setattr("datacrumbs.composite", "")