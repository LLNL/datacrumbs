#!/usr/bin/env python3

"""
Query Chameleon/OpenStack for node names and IP addresses from a reservation id.

Documentation workflow used:
- Chameleon reservations CLI:
  https://chameleoncloud.readthedocs.io/en/latest/technical/reservations/cli_reservations.html
- Chameleon launching instances with reservation hint:
  https://chameleoncloud.readthedocs.io/en/latest/technical/baremetal/launching_cli.html

Prerequisites:
1) Source OpenStack RC script for the project/site.
2) `openstack` CLI must be installed.
3) If reservation subcommands are unavailable, install Chameleon blazar client:
   pip install git+https://github.com/ChameleonCloud/python-blazarclient.git@chameleoncloud/xena
"""

from __future__ import annotations

import argparse
import ast
import getpass
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
from typing import Any


CHI_TACC_ENDPOINTS: dict[str, str] = {
	"Baremetal": "https://chi.tacc.chameleoncloud.org:6385",
	"Baremetal Introspection": "https://chi.tacc.chameleoncloud.org:5050",
	"Cep": "https://chi.tacc.chameleoncloud.org:8910",
	"Cloudformation": "https://chi.tacc.chameleoncloud.org:8000/v1",
	"Compute": "https://chi.tacc.chameleoncloud.org:8774/v2.1",
	"Compute_Legacy": "https://chi.tacc.chameleoncloud.org:8774/v2/946325c85acd4d72a982751218df081f",
	"Computev3": "-",
	"Identity": "https://chi.tacc.chameleoncloud.org:5000",
	"Identityv3": "-",
	"Image": "https://chi.tacc.chameleoncloud.org:9292",
	"Inventory": "https://chi.tacc.chameleoncloud.org:8100",
	"Metering": "-",
	"Metric": "https://chi.tacc.chameleoncloud.org:8041",
	"Network": "https://chi.tacc.chameleoncloud.org:9696",
	"Object Store": "https://chi.tacc.chameleoncloud.org:7480/swift/v1/AUTH_946325c85acd4d72a982751218df081f",
	"Orchestration": "https://chi.tacc.chameleoncloud.org:8004/v1/946325c85acd4d72a982751218df081f",
	"Placement": "https://chi.tacc.chameleoncloud.org:8780",
	"Reservation": "https://chi.tacc.chameleoncloud.org:1234/v1",
	"Share": "https://chi.tacc.chameleoncloud.org:8786/v1/946325c85acd4d72a982751218df081f",
	"Sharev2": "https://chi.tacc.chameleoncloud.org:8786/v2",
}


OPENRC_EXPORT_RE = re.compile(r"^\s*export\s+([A-Za-z_][A-Za-z0-9_]*)=(.*)$")

AUTH_ENV_KEYS_TO_CLEAR = {
	"OS_AUTH_TYPE",
	"OS_AUTH_URL",
	"OS_IDENTITY_API_VERSION",
	"OS_INTERFACE",
	"OS_REGION_NAME",
	"OS_USERNAME",
	"OS_PASSWORD",
	"OS_USER_DOMAIN_ID",
	"OS_USER_DOMAIN_NAME",
	"OS_PROJECT_ID",
	"OS_PROJECT_NAME",
	"OS_PROJECT_DOMAIN_ID",
	"OS_PROJECT_DOMAIN_NAME",
	"OS_DOMAIN_ID",
	"OS_DOMAIN_NAME",
	"OS_TRUST_ID",
	"OS_PROTOCOL",
	"OS_IDENTITY_PROVIDER",
	"OS_DISCOVERY_ENDPOINT",
	"OS_CLIENT_ID",
	"OS_CLIENT_SECRET",
	"OS_ACCESS_TOKEN_TYPE",
	"OS_APPLICATION_CREDENTIAL_ID",
	"OS_APPLICATION_CREDENTIAL_NAME",
	"OS_APPLICATION_CREDENTIAL_SECRET",
}


def strip_shell_quotes(value: str) -> str:
	value = value.strip()
	if len(value) >= 2 and value[0] == value[-1] and value[0] in ('"', "'"):
		return value[1:-1]
	return value


def parse_openrc_exports(openrc_path: Path) -> dict[str, str]:
	result: dict[str, str] = {}
	for raw_line in openrc_path.read_text(encoding="utf-8").splitlines():
		line = raw_line.strip()
		if not line or line.startswith("#"):
			continue
		match = OPENRC_EXPORT_RE.match(line)
		if not match:
			continue
		key, raw_value = match.group(1), match.group(2)
		value = strip_shell_quotes(raw_value)
		result[key] = value
	return result


def find_default_openrc() -> Path | None:
	search_dirs = [Path.cwd(), Path(__file__).resolve().parent]
	for directory in search_dirs:
		for candidate in sorted(directory.glob("CHI-*-openrc.sh")):
			if candidate.is_file():
				return candidate
	return None


def merge_auth_environment(base_env: dict[str, str], openrc_path: Path | None) -> dict[str, str]:
	env = base_env.copy()
	if openrc_path and openrc_path.exists():
		for key in AUTH_ENV_KEYS_TO_CLEAR:
			env.pop(key, None)
		for key, value in parse_openrc_exports(openrc_path).items():
			env[key] = value

	# Application credential authentication must be unscoped.
	has_app_cred = bool(env.get("OS_APPLICATION_CREDENTIAL_ID")) or bool(
		env.get("OS_APPLICATION_CREDENTIAL_NAME") and env.get("OS_APPLICATION_CREDENTIAL_SECRET")
	)
	if has_app_cred:
		env.setdefault("OS_AUTH_TYPE", "v3applicationcredential")
		for scoped_key in (
			"OS_PROJECT_ID",
			"OS_PROJECT_NAME",
			"OS_PROJECT_DOMAIN_ID",
			"OS_PROJECT_DOMAIN_NAME",
			"OS_USER_DOMAIN_ID",
			"OS_USER_DOMAIN_NAME",
			"OS_DOMAIN_ID",
			"OS_DOMAIN_NAME",
			"OS_TRUST_ID",
		):
			env.pop(scoped_key, None)
		# Username/password or OIDC values can conflict with app credential auth.
		for credential_key in (
			"OS_USERNAME",
			"OS_PASSWORD",
			"OS_PROTOCOL",
			"OS_IDENTITY_PROVIDER",
			"OS_DISCOVERY_ENDPOINT",
			"OS_CLIENT_ID",
			"OS_CLIENT_SECRET",
			"OS_ACCESS_TOKEN_TYPE",
		):
			env.pop(credential_key, None)

	# Keystone v3 commonly requires domain context whenever username is provided.
	auth_url = env.get("OS_AUTH_URL", "")
	has_v3_auth_url = "/v3" in auth_url
	username_present = bool(env.get("OS_USERNAME"))
	if has_v3_auth_url and username_present and not has_app_cred:
		env.setdefault("OS_USER_DOMAIN_NAME", "Default")
		env.setdefault("OS_PROJECT_DOMAIN_NAME", "Default")

	if (
		env.get("OS_AUTH_TYPE", "").strip().lower() == "v3oidcpassword"
		and not has_app_cred
		and not env.get("OS_PASSWORD")
	):
		user = env.get("OS_USERNAME", "user")
		env["OS_PASSWORD"] = getpass.getpass(f"({user}) Please enter your Chameleon CLI password: ")

	return env


def resolve_openstack_executable(env: dict[str, str]) -> str:
	cli_override = env.get("OPENSTACK_CLI")
	if cli_override and Path(cli_override).expanduser().exists():
		return str(Path(cli_override).expanduser())

	from_path = shutil.which("openstack", path=env.get("PATH"))
	if from_path:
		return from_path

	python_bin = Path(sys.executable).resolve().parent
	local_candidate = python_bin / "openstack"
	if local_candidate.exists() and local_candidate.is_file():
		return str(local_candidate)

	for candidate in (
		Path.home() / ".local" / "bin" / "openstack",
		Path("/usr/local/bin/openstack"),
		Path("/opt/homebrew/bin/openstack"),
	):
		if candidate.exists() and candidate.is_file():
			return str(candidate)

	raise FileNotFoundError("openstack")


def run_command(cmd: list[str], allow_failure: bool = False, env: dict[str, str] | None = None) -> str:
	proc = subprocess.run(cmd, capture_output=True, text=True, env=env)
	if proc.returncode != 0 and not allow_failure:
		stderr = (proc.stderr or "").strip()
		stdout = (proc.stdout or "").strip()
		if (
			cmd
			and Path(cmd[0]).name == "openstack"
			and "reservation" in cmd
			and "is not an openstack command" in stdout
		):
			raise RuntimeError(
				"OpenStack 'reservation' commands are unavailable in this environment.\n"
				"Install Chameleon's Blazar client plugin:\n"
				"  pip install git+https://github.com/ChameleonCloud/python-blazarclient.git@chameleoncloud/xena\n"
				"Then retry after re-activating the environment."
			)
		if (
			cmd
			and Path(cmd[0]).name == "openstack"
			and "stack" in cmd
			and "is not an openstack command" in stdout
		):
			raise RuntimeError(
				"OpenStack stack commands are unavailable in this environment.\n"
				"Install Heat client plugin:\n"
				"  pip install python-heatclient\n"
				"Then retry after re-activating the environment."
			)
		if stderr and stdout:
			details = f"STDERR:\n{stderr}\n\nSTDOUT:\n{stdout}"
		else:
			details = stderr or stdout or "Unknown error"
		raise RuntimeError(f"Command failed: {' '.join(cmd)}\n{details}")
	return (proc.stdout or "").strip()


def with_openstack_env(base_env: dict[str, str] | None) -> dict[str, str]:
	env = (base_env or os.environ.copy()).copy()
	warning_filter = "ignore:urllib3 v2 only supports OpenSSL 1.1.1+:Warning"
	current = env.get("PYTHONWARNINGS", "").strip()
	if current:
		if warning_filter not in current:
			env["PYTHONWARNINGS"] = f"{warning_filter},{current}"
	else:
		env["PYTHONWARNINGS"] = warning_filter
	return env


def run_openstack_json(
	args: list[str], allow_failure: bool = False, env: dict[str, str] | None = None
) -> Any:
	env = with_openstack_env(env)
	openstack_cli = resolve_openstack_executable(env)
	output = run_command([openstack_cli, *args, "-f", "json"], allow_failure=allow_failure, env=env)
	if not output:
		return []
	try:
		return json.loads(output)
	except json.JSONDecodeError as exc:
		raise RuntimeError(
			f"Expected JSON output from: {openstack_cli} {' '.join(args)} -f json\n"
			f"Got: {output[:300]}"
		) from exc


def parse_embedded(value: Any) -> Any:
	if isinstance(value, (dict, list)):
		return value
	if value is None:
		return None
	if isinstance(value, str):
		text = value.strip()
		if not text:
			return None
		for parser in (json.loads, ast.literal_eval):
			try:
				return parser(text)
			except Exception:
				continue
	return value


def extract_reservations(lease_obj: dict[str, Any]) -> list[dict[str, Any]]:
	reservations = parse_embedded(lease_obj.get("reservations"))
	if isinstance(reservations, list):
		return [entry for entry in reservations if isinstance(entry, dict)]
	return []


def extract_host_ids_from_reservation(reservation: dict[str, Any]) -> set[str]:
	host_ids: set[str] = set()

	for key in ("resource_id", "host_id", "compute_host_id"):
		value = reservation.get(key)
		if isinstance(value, str) and value.strip():
			host_ids.add(value.strip())

	resource_ids = parse_embedded(reservation.get("resource_ids"))
	if isinstance(resource_ids, list):
		for value in resource_ids:
			if isinstance(value, str) and value.strip():
				host_ids.add(value.strip())

	allocations = parse_embedded(reservation.get("allocations"))
	if isinstance(allocations, list):
		for alloc in allocations:
			if not isinstance(alloc, dict):
				continue
			for key in ("resource_id", "host_id", "compute_host_id"):
				value = alloc.get(key)
				if isinstance(value, str) and value.strip():
					host_ids.add(value.strip())

	return host_ids


def fetch_host_ids_from_allocations(
	lease_id: str, reservation_ids: set[str], env: dict[str, str]
) -> set[str]:
	host_ids: set[str] = set()

	lease_commands = [
		["reservation", "host", "allocation", "list", "--lease-id", lease_id],
		["reservation", "host", "allocation", "list", "--lease", lease_id],
	]
	for command in lease_commands:
		rows = run_openstack_json(command, allow_failure=True, env=env)
		if not isinstance(rows, list):
			continue
		for row in rows:
			if not isinstance(row, dict):
				continue
			for key in ("compute_host_id", "host_id", "resource_id", "id"):
				value = row.get(key)
				if isinstance(value, str) and value.strip():
					host_ids.add(value.strip())

	for reservation_id in reservation_ids:
		candidate_commands = [
			["reservation", "host", "allocation", "list", "--reservation-id", reservation_id],
			["reservation", "host", "allocation", "list", "--reservation", reservation_id],
		]
		for command in candidate_commands:
			rows = run_openstack_json(command, allow_failure=True, env=env)
			if not isinstance(rows, list):
				continue
			for row in rows:
				if not isinstance(row, dict):
					continue
				for key in ("compute_host_id", "host_id", "resource_id", "id"):
					value = row.get(key)
					if isinstance(value, str) and value.strip():
						host_ids.add(value.strip())
	return host_ids


def fetch_hosts_from_lease(lease_id: str, env: dict[str, str]) -> list[dict[str, Any]]:
	hosts: list[dict[str, Any]] = []
	for command in (
		["reservation", "host", "list", "--lease-id", lease_id],
		["reservation", "host", "list", "--lease", lease_id],
	):
		rows = run_openstack_json(command, allow_failure=True, env=env)
		if isinstance(rows, list) and rows:
			for row in rows:
				if isinstance(row, dict):
					hosts.append(row)
	return hosts


def fetch_hosts_by_global_filter(
	lease_id: str, reservation_ids: set[str], env: dict[str, str]
) -> list[dict[str, Any]]:
	rows = run_openstack_json(["reservation", "host", "list"], allow_failure=True, env=env)
	if not isinstance(rows, list):
		return []

	identifiers = {lease_id, *reservation_ids}
	filtered: list[dict[str, Any]] = []
	for row in rows:
		if not isinstance(row, dict):
			continue
		blob = json.dumps(row, default=str)
		if any(identifier in blob for identifier in identifiers):
			filtered.append(row)
	return filtered


def parse_ips(server_details: dict[str, Any]) -> list[str]:
	ip_values: list[str] = []

	addresses = parse_embedded(server_details.get("addresses") or server_details.get("Addresses"))
	if isinstance(addresses, dict):
		for entries in addresses.values():
			if not isinstance(entries, list):
				continue
			for item in entries:
				if isinstance(item, dict):
					addr = item.get("addr")
					if isinstance(addr, str) and addr not in ip_values:
						ip_values.append(addr)
				elif isinstance(item, str):
					ip = item.strip()
					if ip and ip not in ip_values:
						ip_values.append(ip)

	for key in ("accessIPv4", "accessIPv6"):
		value = server_details.get(key)
		if isinstance(value, str):
			ip = value.strip()
			if ip and ip not in ip_values:
				ip_values.append(ip)

	if ip_values:
		return ip_values

	networks = server_details.get("Networks") or server_details.get("networks")
	if isinstance(networks, str):
		parts = [part.strip() for part in networks.split(",")]
		for part in parts:
			if "=" in part:
				_, value = part.split("=", 1)
				ip = value.strip()
			else:
				ip = part.strip()
			if ip and ip not in ip_values:
				ip_values.append(ip)

	return ip_values


def reservation_hint_matches(server_details: dict[str, Any], reservation_ids: set[str]) -> bool:
	hint_fields = [
		server_details.get("OS-SCH-HNT:scheduler_hints"),
		server_details.get("scheduler_hints"),
		server_details.get("properties"),
		server_details.get("Properties"),
	]
	for field in hint_fields:
		parsed = parse_embedded(field)
		if isinstance(parsed, dict):
			for key in ("reservation", "reservation_id"):
				value = parsed.get(key)
				if isinstance(value, str) and value.strip() in reservation_ids:
					return True
		if isinstance(parsed, str) and any(identifier in parsed for identifier in reservation_ids):
				return True

	text_blob = json.dumps(server_details, default=str)
	return any(identifier in text_blob for identifier in reservation_ids)


def get_node_name(server_details: dict[str, Any]) -> str:
	for key in ("OS-EXT-SRV-ATTR:hypervisor_hostname", "hypervisor_hostname", "Host", "host"):
		value = server_details.get(key)
		if isinstance(value, str) and value.strip():
			return value.strip()
	return ""


def find_servers_for_reservation(
	reservation_ids: set[str], host_identifiers: set[str], env: dict[str, str]
) -> list[dict[str, Any]]:
	rows = run_openstack_json(["server", "list", "--long"], env=env)
	if not isinstance(rows, list):
		return []

	servers: list[dict[str, Any]] = []
	all_server_details: list[dict[str, Any]] = []
	for row in rows:
		if not isinstance(row, dict):
			continue
		server_id = row.get("ID") or row.get("id")
		if not isinstance(server_id, str) or not server_id.strip():
			continue
		details = run_openstack_json(["server", "show", server_id.strip()], env=env)
		if not isinstance(details, dict):
			continue
		all_server_details.append(details)

		if reservation_hint_matches(details, reservation_ids):
			servers.append(details)
			continue

		server_host_values = {
			str(details.get("OS-EXT-SRV-ATTR:host") or "").strip(),
			str(details.get("OS-EXT-SRV-ATTR:hypervisor_hostname") or "").strip(),
			str(details.get("host") or "").strip(),
			str(details.get("hypervisor_hostname") or "").strip(),
		}
		server_host_values = {item for item in server_host_values if item}
		if host_identifiers and server_host_values.intersection(host_identifiers):
			servers.append(details)

	if not servers:
		# Heuristic fallback: if strict reservation linkage is absent, surface likely
		# instances by lease-id token or lease-name token in server name/properties.
		for details in all_server_details:
			blob = json.dumps(details, default=str).lower()
			if any(identifier.lower() in blob for identifier in reservation_ids):
				servers.append(details)
	return servers


def add_lease_name_heuristic(
	servers: list[dict[str, Any]], all_candidates: list[dict[str, Any]], lease_name: str
) -> list[dict[str, Any]]:
	if servers or not lease_name:
		return servers

	tokens = [token for token in re.split(r"[^a-zA-Z0-9]+", lease_name.lower()) if len(token) >= 3]
	if not tokens:
		return servers

	server_ids = {str(item.get("id") or item.get("ID") or "").strip() for item in servers}
	for details in all_candidates:
		server_name = str(details.get("name") or details.get("Name") or "").lower()
		if any(token in server_name for token in tokens):
			server_id = str(details.get("id") or details.get("ID") or "").strip()
			if server_id and server_id not in server_ids:
				servers.append(details)
				server_ids.add(server_id)
	return servers


def get_all_server_details(env: dict[str, str]) -> list[dict[str, Any]]:
	rows = run_openstack_json(["server", "list", "--long"], env=env)
	if not isinstance(rows, list):
		return []

	details_list: list[dict[str, Any]] = []
	for row in rows:
		if not isinstance(row, dict):
			continue
		server_id = row.get("ID") or row.get("id")
		if not isinstance(server_id, str) or not server_id.strip():
			continue
		details = run_openstack_json(["server", "show", server_id.strip()], env=env, allow_failure=True)
		if isinstance(details, dict):
			details_list.append(details)
	return details_list


def stack_matches_server(server_details: dict[str, Any], stack_id: str, stack_name: str) -> bool:
	properties = parse_embedded(server_details.get("properties") or server_details.get("Properties"))
	if isinstance(properties, dict):
		stack_id_fields = [
			properties.get("OS::stack_id"),
			properties.get("stack_id"),
		]
		if any(isinstance(value, str) and value.strip() == stack_id for value in stack_id_fields):
			return True

		stack_name_fields = [
			properties.get("OS::stack_name"),
			properties.get("stack_name"),
		]
		if any(isinstance(value, str) and value.strip() == stack_name for value in stack_name_fields):
			return True

	text_blob = json.dumps(server_details, default=str)
	return stack_id in text_blob or stack_name in text_blob


def build_stack_result(stack_ref: str, env: dict[str, str]) -> dict[str, Any]:
	stack = run_openstack_json(["stack", "show", stack_ref], env=env)
	if not isinstance(stack, dict):
		raise RuntimeError("Failed to parse stack details from OpenStack")

	stack_id = str(stack.get("id") or stack.get("ID") or "").strip()
	stack_name = str(stack.get("stack_name") or stack.get("stack_name") or stack.get("name") or stack_ref)
	stack_status = str(stack.get("stack_status") or stack.get("status") or "")

	all_servers = get_all_server_details(env)
	servers = [
		server
		for server in all_servers
		if isinstance(server, dict) and stack_matches_server(server, stack_id, stack_name)
	]

	server_rows: list[dict[str, Any]] = []
	seen_server_ids: set[str] = set()
	host_rows: list[dict[str, str]] = []
	seen_host_keys: set[str] = set()
	for server in servers:
		server_id = str(server.get("id") or server.get("ID") or "").strip()
		if not server_id or server_id in seen_server_ids:
			continue
		seen_server_ids.add(server_id)
		node_name = get_node_name(server)
		ips = parse_ips(server)
		server_rows.append(
			{
				"server_id": server_id,
				"server_name": server.get("name") or server.get("Name") or "",
				"node": node_name,
				"ips": ips,
				"status": server.get("status") or server.get("Status") or "",
			}
		)

		host_key = node_name or server_id
		if host_key not in seen_host_keys:
			seen_host_keys.add(host_key)
			host_rows.append(
				{
					"host_id": node_name or "",
					"node": node_name or "",
					"uid": "",
				}
			)

	return {
		"reservation_id": "",
		"lease_name": "",
		"lease_status": "",
		"stack_id": stack_id,
		"stack_name": stack_name,
		"stack_status": stack_status,
		"hosts": host_rows,
		"servers": server_rows,
	}


def fetch_host_details(host_ids: set[str], env: dict[str, str]) -> list[dict[str, Any]]:
	host_details: list[dict[str, Any]] = []
	for host_id in sorted(host_ids):
		details = run_openstack_json(
			["reservation", "host", "show", host_id], allow_failure=True, env=env
		)
		if isinstance(details, dict):
			host_details.append(details)
	return host_details


def build_result(reservation_id: str, env: dict[str, str]) -> dict[str, Any]:
	lease = run_openstack_json(["reservation", "lease", "show", reservation_id], env=env)
	if not isinstance(lease, dict):
		raise RuntimeError("Failed to parse lease details from OpenStack")

	reservations = extract_reservations(lease)

	reservation_entry_ids: set[str] = set()
	host_ids: set[str] = set()
	for reservation in reservations:
		reservation_type = reservation.get("resource_type")
		if reservation_type != "physical:host":
			continue
		reservation_entry_id = reservation.get("id")
		if isinstance(reservation_entry_id, str) and reservation_entry_id.strip():
			reservation_entry_ids.add(reservation_entry_id.strip())
		host_ids.update(extract_host_ids_from_reservation(reservation))

	if reservation_entry_ids:
		host_ids.update(fetch_host_ids_from_allocations(reservation_id, reservation_entry_ids, env))

	lease_hosts = fetch_hosts_from_lease(reservation_id, env)
	for lease_host in lease_hosts:
		for key in ("id", "host_id", "compute_host_id", "resource_id", "uid", "hypervisor_hostname"):
			value = lease_host.get(key)
			if isinstance(value, str) and value.strip():
				host_ids.add(value.strip())

	reservation_match_ids = {reservation_id, *reservation_entry_ids}
	hosts = fetch_host_details(host_ids, env)

	host_identifiers: set[str] = set(host_ids)
	for host in hosts:
		for key in ("id", "uid", "hypervisor_hostname", "name"):
			value = host.get(key)
			if isinstance(value, str) and value.strip():
				host_identifiers.add(value.strip())
	for lease_host in lease_hosts:
		for key in ("id", "uid", "hypervisor_hostname", "name", "host_id", "compute_host_id"):
			value = lease_host.get(key)
			if isinstance(value, str) and value.strip():
				host_identifiers.add(value.strip())

	for lease_host in fetch_hosts_by_global_filter(reservation_id, reservation_entry_ids, env):
		for key in ("id", "uid", "hypervisor_hostname", "name", "host_id", "compute_host_id"):
			value = lease_host.get(key)
			if isinstance(value, str) and value.strip():
				host_identifiers.add(value.strip())
		lease_hosts.append(lease_host)

	servers = find_servers_for_reservation(reservation_match_ids, host_identifiers, env)
	if not servers:
		servers = add_lease_name_heuristic(servers, get_all_server_details(env), str(lease.get("name") or ""))

	server_rows: list[dict[str, Any]] = []
	seen_server_ids: set[str] = set()
	for server in servers:
		server_id = str(server.get("id") or server.get("ID") or "").strip()
		if not server_id or server_id in seen_server_ids:
			continue
		seen_server_ids.add(server_id)
		server_rows.append(
			{
				"server_id": server_id,
				"server_name": server.get("name") or server.get("Name") or "",
				"node": get_node_name(server),
				"ips": parse_ips(server),
				"status": server.get("status") or server.get("Status") or "",
			}
		)

	host_rows: list[dict[str, str]] = []
	seen_host_ids: set[str] = set()
	for host in hosts:
		host_id = str(host.get("id") or "")
		if host_id:
			seen_host_ids.add(host_id)
		host_rows.append(
			{
				"host_id": host_id,
				"node": str(host.get("hypervisor_hostname") or host.get("name") or ""),
				"uid": str(host.get("uid") or ""),
			}
		)

	for lease_host in lease_hosts:
		host_id = str(
			lease_host.get("id")
			or lease_host.get("host_id")
			or lease_host.get("compute_host_id")
			or ""
		)
		if not host_id or host_id in seen_host_ids:
			continue
		seen_host_ids.add(host_id)
		host_rows.append(
			{
				"host_id": host_id,
				"node": str(lease_host.get("hypervisor_hostname") or lease_host.get("name") or ""),
				"uid": str(lease_host.get("uid") or ""),
			}
		)

	for host_id in sorted(host_ids):
		if host_id in seen_host_ids:
			continue
		host_rows.append(
			{
				"host_id": host_id,
				"node": "",
				"uid": "",
			}
		)

	return {
		"reservation_id": reservation_id,
		"lease_name": lease.get("name") or "",
		"lease_status": lease.get("status") or "",
		"hosts": host_rows,
		"servers": server_rows,
	}


def print_human(result: dict[str, Any], include_instances: bool = False) -> None:
	if result.get("stack_id") or result.get("stack_name"):
		print(f"Stack      : {result.get('stack_name', '')}")
		if result.get("stack_id"):
			print(f"Stack ID   : {result['stack_id']}")
		if result.get("stack_status"):
			print(f"Stack State: {result['stack_status']}")
	else:
		print(f"Reservation: {result.get('reservation_id', '')}")
		if result.get("lease_name"):
			print(f"Lease Name : {result['lease_name']}")
		if result.get("lease_status"):
			print(f"Lease State: {result['lease_status']}")

	print("\nAllocated nodes in lease:")
	hosts = result.get("hosts", [])
	if not hosts:
		print("  (none found)")
	else:
		for host in hosts:
			print(
				f"  - host_id={host.get('host_id', '')} "
				f"node={host.get('node', 'unknown')} uid={host.get('uid', '')}"
			)

	if include_instances:
		print("\nLaunched instances (optional view):")
		servers = result.get("servers", [])
		if not servers:
			print("  (none found)")
		else:
			for server in servers:
				ips = ", ".join(server.get("ips", [])) if server.get("ips") else "n/a"
				print(
					f"  - {server.get('server_name', '')} ({server.get('server_id', '')}) "
					f"node={server.get('node', 'n/a')} status={server.get('status', 'n/a')} ips={ips}"
				)


def parse_args() -> argparse.Namespace:
	parser = argparse.ArgumentParser(
		description="Get node list and IP addresses from a Chameleon reservation id"
	)
	parser.add_argument(
		"reservation_id",
		nargs="?",
		help="Reservation ID (or lease identifier accepted by 'openstack reservation lease show')",
	)
	parser.add_argument(
		"--stack",
		help="OpenStack Heat stack name or ID to query deployed nodes and IPs",
	)
	parser.add_argument(
		"--json",
		action="store_true",
		help="Print machine-readable JSON output",
	)
	parser.add_argument(
		"--include-instances",
		action="store_true",
		help="Include launched instance details in human-readable output",
	)
	parser.add_argument(
		"--print-endpoints",
		action="store_true",
		help="Print CHI@TACC service endpoints and exit",
	)
	parser.add_argument(
		"--use-chi-tacc-auth-url",
		action="store_true",
		help="If OS_AUTH_URL is not already set, use CHI@TACC Identity endpoint",
	)
	parser.add_argument(
		"--openrc",
		help="Path to an OpenStack RC file to import auth variables (default: auto-detect CHI-*-openrc.sh)",
	)
	return parser.parse_args()


def print_chi_tacc_endpoints() -> None:
	print("CHI@TACC service endpoints:")
	for service, endpoint in CHI_TACC_ENDPOINTS.items():
		print(f"- {service}: {endpoint}")


def main() -> int:
	args = parse_args()
	if args.print_endpoints:
		print_chi_tacc_endpoints()
		return 0

	if args.stack and args.reservation_id:
		print("Error: provide either reservation_id or --stack, not both", file=sys.stderr)
		return 2

	if not args.reservation_id and not args.stack:
		print(
			"Error: reservation_id or --stack is required unless --print-endpoints is used",
			file=sys.stderr,
		)
		return 2

	env = os.environ.copy()
	if args.use_chi_tacc_auth_url and not env.get("OS_AUTH_URL"):
		env["OS_AUTH_URL"] = CHI_TACC_ENDPOINTS["Identity"]

	openrc_path: Path | None = None
	if args.openrc:
		openrc_path = Path(args.openrc).expanduser().resolve()
		if not openrc_path.exists():
			print(f"Error: openrc file not found: {openrc_path}", file=sys.stderr)
			return 2
	else:
		openrc_path = find_default_openrc()

	env = merge_auth_environment(env, openrc_path)

	try:
		if args.stack:
			result = build_stack_result(args.stack, env)
		else:
			result = build_result(str(args.reservation_id), env)
		if args.json:
			print(json.dumps(result, indent=2))
		else:
			print_human(result, include_instances=args.include_instances)
		return 0
	except RuntimeError as exc:
		print(f"Error: {exc}", file=sys.stderr)
		if "must also provide either a user_domain_id or user_domain_name" in str(exc):
			print(
				"Hint: source your OpenStack RC file or pass --openrc <path>.\n"
				"For Keystone v3 username auth, ensure OS_USER_DOMAIN_NAME and OS_PROJECT_DOMAIN_NAME are set.",
				file=sys.stderr,
			)
		return 1
	except FileNotFoundError:
		print(
			"Error: OpenStack CLI was not found.\n"
			"Install it in your current Python environment:\n"
			"  pip install python-openstackclient\n"
			"For Chameleon reservation commands, also install:\n"
			"  pip install git+https://github.com/ChameleonCloud/python-blazarclient.git@chameleoncloud/xena\n"
			"Then either activate that environment or set OPENSTACK_CLI to the executable path.",
			file=sys.stderr,
		)
		return 1


if __name__ == "__main__":
	raise SystemExit(main())
