import git
import json
import os
import shutil

DST = "../sources"
mapping_file = open("./ClassMap/classMap.json")
mapping = json.load(mapping_file)

os.system("git config --global core.protectNTFS false")
for project_name, project in mapping['projects'].items():

    if "srcURL" in project and project["srcURL"]["type"] == "git":
        if os.path.isdir(os.path.join(DST, project_name)):
            continue

        try:
            print("Processing ", project_name)
            repo = git.Git(DST).clone(project["srcURL"]["URL"])
            repo = git.Repo(f"{DST}/{project_name}")
            for submodule in repo.submodules:
                submodule.update(init=True)
        except:
            pass
        project_root = os.path.basename(os.path.normpath(project["srcURL"]["URL"]))
        if project_root != project_name:
            os.rename(project_root, project_name)
        if "delete" in project:
            for dir_to_delete in project["delete"]:
                shutil.rmtree(f"{DST}/{project_name}/{dir_to_delete}")
        # TODO: delete unnecessary files in each repo