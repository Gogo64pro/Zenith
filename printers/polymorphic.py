import gdb
import re

class SimpleZenithPolymorphicPrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            # Get the shared_ptr
            ptr = self.val['ptr_']
            if not ptr:
                return "zenith::polymorphic<null>"

            # Get the pointer value
            data_ptr = ptr['_M_ptr']
            if not data_ptr:
                return "zenith::polymorphic<null>"

            # Get base type
            base_type = str(self.val.type.template_argument(0))

            # Get address
            addr = hex(int(data_ptr))

            # Try to get actual type
            try:
                actual_type = data_ptr.dynamic_type
                actual_name = str(actual_type)

                # Simplify type names
                if 'zenith::' in actual_name:
                    actual_name = actual_name.split('::')[-1].split('<')[0]

                return f"zenith::polymorphic<{base_type}>({actual_name}@{addr})"
            except:
                return f"zenith::polymorphic<{base_type}>(@{addr})"

        except Exception as e:
            return f"zenith::polymorphic<error: {str(e)}>"

    def children(self):
        try:
            ptr = self.val['ptr_']
            if not ptr:
                return []

            data_ptr = ptr['_M_ptr']
            if not data_ptr:
                return []

            # Return the actual object as a single child
            # This lets CLion expand it normally
            yield ("[object]", data_ptr.dereference())

            # Add refcount if available
            try:
                refcount = ptr['_M_refcount']
                # Try to get counts
                if '_M_pi' in refcount.type.fields():
                    pi = refcount['_M_pi']
                    if pi:
                        yield ("[refcount]", str(pi.dereference()))
            except:
                pass

        except Exception as e:
            yield ("[error]", str(e))

# Register function
def build_pretty_printer():
    pp = gdb.printing.RegexpCollectionPrettyPrinter("zenith_polymorphic")
    pp.add_printer('zenith::polymorphic', '^zenith::polymorphic<.*>$', SimpleZenithPolymorphicPrinter)
    return pp

# Register it
gdb.printing.register_pretty_printer(
    gdb.current_objfile(),
    build_pretty_printer()
)