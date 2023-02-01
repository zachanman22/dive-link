from bluebuzz_config import Bluebuzz_Config

bluz = Bluebuzz_Config()
print(bluz.hamming_enabled)
bluz.hamming_enabled = False
print(repr(bluz))
bluz.baud = 100
print(repr(bluz))
bluz.expected_message_type = 'hello'
print(repr(bluz))