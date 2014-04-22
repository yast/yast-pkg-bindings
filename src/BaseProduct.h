/* 
 * File:   BaseProduct.h
 * Author: lslezak@suse.cz
 *
 * Remember the attributes of the base product for creating the
 * /etc/products.d/baseproduct symlink.
 *
 * (Note: zypp::Product reference cannot be used as it might be invalidated
 * after adding/removing repositories or at target reload after commit.)
 */

#ifndef BASEPRODUCT_H
#define BASEPRODUCT_H

#include <string>
#include <zypp/Edition.h>
#include <zypp/Arch.h>

class BaseProduct {

public:
  BaseProduct(
    const std::string& product_name,
    const zypp::Edition& product_edition,
    const zypp::Arch& product_arch,
    const std::string& source_repo_alias
  );

  std::string name;
  // zypp::Edition contains both version and release
  zypp::Edition edition;
  zypp::Arch arch;
  std::string repo_alias;
};

#endif	/* BASEPRODUCT_H */

